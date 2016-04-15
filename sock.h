void error(const char *msg)
{
    perror(msg);
    exit(0);
}

#define SOCKADDR_LEN sizeof(struct sockaddr_in)
// socklen_t length = sizeof(struct sockaddr_in);

void setup_host_sockaddr(struct sockaddr_in &addr, char *hostname, int port )
{
    struct hostent *host;
    host = gethostbyname(hostname);
    if (host == NULL) error("Unknown host");
    bzero((char *) &addr, sizeof(addr));
    addr.sin_family=AF_INET; //~~ AF_UNSPEC
    bcopy((char *) host->h_addr, (char *) &addr.sin_addr.s_addr, host->h_length);
    addr.sin_port=htons(port);
}

void setup_local_sockaddr(struct sockaddr_in &addr, int port)
{
    bzero((char *) &addr, SOCKADDR_LEN);
    addr.sin_family = AF_INET; // server byte order
    addr.sin_addr.s_addr = INADDR_ANY; // automatically be filled with current host's IP address
    addr.sin_port = htons(port); // convert short integer value for port must be converted into network byte order
}

//  bind the socket to the current IP address on port
void bind_socket(int sockfd, int port)
{
    struct sockaddr_in addr;
    setup_local_sockaddr(addr, port);
        // though every time we bind a socket, the local addr is reconstructed, it is trivial
    if (bind(sockfd, (struct sockaddr *)&addr, SOCKADDR_LEN)<0)
        error("ERROR on binding");

}