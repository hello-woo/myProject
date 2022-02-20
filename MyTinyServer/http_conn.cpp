#include "http_conn.h"


int http_conn::m_epollfd = -1;
int http_conn::m_user_count = 0;

const char* doc_root = "/home/zzc/myProject/MyTinyServer/resource";

void setnoblocking(int fd){
    int old_flag = fcntl(fd,F_GETFL);
    int new_flag = old_flag |O_NONBLOCK;
    fcntl(fd,F_SETFL,new_flag);//设置
}

//向epoll中添加需要监听的文件描述符
void addfd(int epollfd,int fd,bool one_shot){
    epoll_event event;
    event.data.fd = fd;
    //event.events = EPOLLIN|EPOLLRDHUP; // REVEC ,READ,直接通过信号
    event.events = EPOLLIN|EPOLLET |EPOLLRDHUP; // 边缘触发
    if(one_shot){
        event.events | EPOLLONESHOT;
    }
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    //设置文件描述符非阻塞·；
    setnoblocking(fd);
}


//从epoll中移除监听的文件描述符
void removefd(int epollfd,int fd){
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,0);
    close(fd);
}

//修改文件描述符,重置socket上的epolloneshot事件，以确保下一次可读。Epollin事件可以触发
void modfd(int epollfd,int fd,int ev){
    epoll_event event;
    event.data.fd = fd;
    //event.events = ev|EPOLLONESHOT|EPOLLRDHUP;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&event);
}

//初始化连接
void http_conn::init(int sockfd,const sockaddr_in& addr){
    m_sockfd = sockfd;
    m_address = addr;
    //设置端口复用
    int reuse = 1;
    setsockopt(m_sockfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));

    //添加到epoll对象
    addfd(m_epollfd,sockfd,true);
    m_user_count++;

    init();
}

void http_conn::init(){
    m_check_state = CHECK_STATE_REQUESTLINE;//初始化状态为解析请求首行
    m_checked_index = 0;
    m_start_line = 0;
    m_read_idx = 0;

    m_method = GET;
    m_url = 0;
    m_version = 0;
    m_linger = false;

    bzero(m_read_buf,READ_BUFFER_SIZE);
}

//关闭连接
void http_conn::close_conn(){
    if(m_sockfd != -1){
        removefd(m_epollfd,m_sockfd);
        m_sockfd = -1;
        m_user_count--; //关闭连接，客户减一
    }
}

//循环读取数据，知道无数据或者对方关闭连接
bool http_conn::read(){
    if(m_read_idx >= READ_BUFFER_SIZE){
        return false;
    }
    //读取到的字节
    int bytes_read = 0;
    while(true){
        bytes_read = recv(m_sockfd,m_read_buf + m_read_idx,READ_BUFFER_SIZE - m_read_idx,0);
        if(bytes_read == -1){
            if(errno == EAGAIN||errno == EWOULDBLOCK){
                //没有数据
                break;
            }
            return false;
        }else if(bytes_read == 0){
            //对方关闭连接
            return false;
        }
        m_read_idx += bytes_read;
    }
    printf("读到数据：%s\n",m_read_buf);
    return true;
}
//主状态机，解析请求
http_conn::HTTP_CODE http_conn::process_read(){
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;

    char *text = 0;
    while(((m_check_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK))
    ||(line_status = parse_line()) == LINE_OK){
        //解析到了一行完整的数据，或者解析到了请求体，也是完整的数据
        //获取一行数据
        text = get_line();

        m_start_line = m_checked_index;
        printf("got one row of http :%s\n",text);

        switch (m_check_state)
        {
        case CHECK_STATE_REQUESTLINE:
        {
            ret = parse_request_line(text);
            if(ret == BAD_REQUEST){
                return BAD_REQUEST;
            }
            break;
        }
        case CHECK_STATE_HEADER:
        {
            ret = parse_request_headers(text);
            if(ret == BAD_REQUEST){
                return BAD_REQUEST;
            }else if(ret == GET_REQUEST){
                return do_request();
            }
        }
        case CHECK_STATE_CONTENT:
        {
            ret = parse_content(text);
            if(ret == GET_REQUEST){
                return do_request();
            }
            line_status = LINE_OPEN;
            break;

        }
        default:
            return INTERNAL_ERROR;
            break;
        }
    }
    return NO_REQUEST;
}
//解析一行\r\n
http_conn::LINE_STATUS http_conn::parse_line(){
    char temp;
    for(;m_checked_index < m_read_idx; ++m_checked_index){
        temp = m_read_buf[m_checked_index];
        if(temp == '\r'){
            if((m_checked_index + 1) == m_read_idx){
                return LINE_OPEN;
            }else if(m_read_buf[m_checked_index + 1] == '\n'){
                m_read_buf[m_checked_index++] = '\0';
                m_read_buf[m_checked_index++] = '\0';
                return LINE_OK;
            }
        }else if(temp == '\n'){
            if((m_checked_index > 1) && (m_read_buf[m_checked_index - 1] == '\r')){
                m_read_buf[m_checked_index - 1] = '\0';
                m_read_buf[m_checked_index++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}
//解析HTTP请求首行
//GET /url HTTP/1.1 可以考虑使用正则表达式
http_conn::HTTP_CODE http_conn::parse_request_line(char *text)
{
    m_url = strpbrk(text, " \t");
    if (!m_url)
    {
        return BAD_REQUEST;
    }
    //GET\0/url HTTP/1.1
    *m_url++ = '\0';
    char *method = text;
    if (strcasecmp(method, "GET") == 0)
        m_method = GET;
    else
        return BAD_REQUEST;
    m_url += strspn(m_url, " \t");
    ///url HTTP/1.1
    m_version = strpbrk(m_url, " \t");
    if (!m_version)
        return BAD_REQUEST;
    *m_version++ = '\0';
    m_version += strspn(m_version, " \t");
    if (strcasecmp(m_version, "HTTP/1.1") != 0)
        return BAD_REQUEST;
    // http://192.168.1.1:1000/index.html
    if (strncasecmp(m_url, "http://", 7) == 0)
    {
        m_url += 7; // 192.168.1.1:1000/index.html
        m_url = strchr(m_url, '/'); // /index.html
    }

    if (strncasecmp(m_url, "https://", 8) == 0)
    {
        m_url += 8;
        m_url = strchr(m_url, '/');
    }

    if (!m_url || m_url[0] != '/')
        return BAD_REQUEST;
    //当url为/时，显示判断界面
    if (strlen(m_url) == 1)
        strcat(m_url, "judge.html");
    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}


 //解析请求头
//解析http请求的一个头部信息
http_conn::HTTP_CODE http_conn::parse_request_headers(char *text)
{
    //遇到空行，表示头部字段解析完毕
    if (text[0] == '\0')
    {
        //如果有HTTP请求消息体，一定要读取消息体的长度，将状态机状态转移到CHECK_STATE_CONTENT状态
        if (m_content_length != 0)
        {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        //得到一个完整的请求
        return GET_REQUEST;
    }
    else if (strncasecmp(text, "Connection:", 11) == 0)
    {
        //处理connection头部字段
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0)
        {
            m_linger = true;
        }
    }
    else if (strncasecmp(text, "Content-length:", 15) == 0)
    {
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atol(text);
    }
    else if (strncasecmp(text, "Host:", 5) == 0)
    {
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
    }
    else
    {
        printf("oop!unknow header: %s\n",text);
        //LOG_INFO("oop!unknow header: %s", text);
        //Log::get_instance()->flush();
    }
    return NO_REQUEST;
}

//解析请求体--判断它是否被完整的读入了
http_conn::HTTP_CODE http_conn::parse_content(char * text){
    if (m_read_idx >= (m_content_length + m_checked_index))
    {
        text[m_content_length] = '\0';
        //POST请求中最后为输入的用户名和密码
        //m_string = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

// 当得到一个完整、正确的HTTP请求时，我们就分析目标文件的属性，
// 如果目标文件存在、对所有用户可读，且不是目录，则使用mmap将其
// 映射到内存地址m_file_address处，并告诉调用者获取文件成功
http_conn::HTTP_CODE http_conn::do_request()
{
    // "/home/nowcoder/webserver/resources"
    strcpy( m_real_file, doc_root );
    int len = strlen( doc_root );
    strncpy( m_real_file + len, m_url, FILENAME_LEN - len - 1 );
    // 获取m_real_file文件的相关的状态信息，-1失败，0成功
    if ( stat( m_real_file, &m_file_stat ) < 0 ) {
        return NO_RESOURCE;
    }

    // 判断访问权限
    if ( ! ( m_file_stat.st_mode & S_IROTH ) ) {
        return FORBIDDEN_REQUEST;
    }

    // 判断是否是目录
    if ( S_ISDIR( m_file_stat.st_mode ) ) {
        return BAD_REQUEST;
    }

    // 以只读方式打开文件
    int fd = open( m_real_file, O_RDONLY );
    // 创建内存映射
    m_file_address = ( char* )mmap( 0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0 );
    close( fd );
    return FILE_REQUEST;
}

// 对内存映射区执行munmap操作
void http_conn::unmap() {
    if( m_file_address )
    {
        munmap( m_file_address, m_file_stat.st_size );
        m_file_address = 0;
    }
}

// 写HTTP响应
bool http_conn::write()
{
    int temp = 0;
    
    if ( bytes_to_send == 0 ) {
        // 将要发送的字节为0，这一次响应结束。
        modfd( m_epollfd, m_sockfd, EPOLLIN ); 
        init();
        return true;
    }

    while(1) {
        // 分散写
        temp = writev(m_sockfd, m_iv, m_iv_count);
        if ( temp <= -1 ) {
            // 如果TCP写缓冲没有空间，则等待下一轮EPOLLOUT事件，虽然在此期间，
            // 服务器无法立即接收到同一客户的下一个请求，但可以保证连接的完整性。
            if( errno == EAGAIN ) {
                modfd( m_epollfd, m_sockfd, EPOLLOUT );
                return true;
            }
            unmap();
            return false;
        }

        bytes_have_send += temp;
        bytes_to_send -= temp;

        if (bytes_have_send >= m_iv[0].iov_len)
        {
            m_iv[0].iov_len = 0;
            m_iv[1].iov_base = m_file_address + (bytes_have_send - m_write_idx);
            m_iv[1].iov_len = bytes_to_send;
        }
        else
        {
            m_iv[0].iov_base = m_write_buf + bytes_have_send;
            m_iv[0].iov_len = m_iv[0].iov_len - temp;
        }

        if (bytes_to_send <= 0)
        {
            // 没有数据要发送了
            unmap();
            modfd(m_epollfd, m_sockfd, EPOLLIN);

            if (m_linger)
            {
                init();
                return true;
            }
            else
            {
                return false;
            }
        }

    }
}

//线程池中额工作线程调用，这是处理HTTP请求的入口函数
void http_conn::process(){
    //解析HTTP请求
    HTTP_CODE read_ret= process_read();
    if(read_ret == NO_REQUEST){
        modfd(m_epollfd,m_sockfd,EPOLLIN);
        return;
    }
    printf("parse requset,create response\n");
    //生成响应
    bool write_ret = process
}