// getaddrinfo.cpp : Defines the entry point for the console application.
// rlum  200120829
//

#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h> // inet_ntop
#include <unistd.h> // gethostname
#endif


#define HOSTNAMESZ 128
#define VERBOSE 1
#define DEBUG if (VERBOSE) printf

#ifdef WIN32
  WSADATA WSStartData;  

void initialize_sockets()
{
  int rc = WSAStartup(MAKEWORD (2,0),&WSStartData);
  DEBUG("WSAStartData.szDescription = %s\n",WSStartData.szDescription);
  DEBUG("WSAStartData.szSystemStatus = %s\n", WSStartData.szSystemStatus);
  DEBUG("WSAStartData.iMaxSockets = %d\n", WSStartData.iMaxSockets);
  switch (rc){
    case 0:
      //all ok
      break;
    case WSASYSNOTREADY:
      printf("WSASYSNOTREADY: The underlying network subsystem is not ready for network communication.\n");
      break;
    case WSAVERNOTSUPPORTED:
      printf("WSAVERNOTSUPPORTED: The version of Windows Sockets support requested is not provided by this particular Sockets implementation\n");
      break;
    case WSAEINPROGRESS:
      printf("WSAEINPROGRESS: A blocking Windows Sockets 1.1 operation is in progress\n");
      break;
    case WSAEPROCLIM:
      printf("WSAEPROCLIM: A limit on the number of tasks supported by the Windows Sockets implementation has been reached.\n");
      break;
    case WSAEFAULT:
      printf("WSAEFAULT: The lpWSAData parameter is not a valid pointer.\n");
      break;
    default:
      printf("WSAStartUp error code %d\n", rc);
      break;
  }
}

void cleanup_sockets()
{
    char running[] ="Running";
    if (strncmp(running,WSStartData.szSystemStatus, strlen(running))==0){
      DEBUG("WSA is running, cleaning up");
      WSACleanup();
    }else{
      DEBUG("WSA is '%s' %d, no need to clean up",WSStartData.szSystemStatus,strlen(WSStartData.szSystemStatus));
    }
}

// formatmessage returns a wchar* 
wchar_t* wsaerrorstr(int errnumber){
  DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
  LPCVOID lpSource = NULL;
  DWORD dwMessageId = errnumber;  // WSAGetLastError();
  DWORD dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
  LPWSTR lpBuffer=NULL;
  DWORD nSize = 0;
  va_list *Args = NULL;

  DWORD buffsize  = FormatMessage( dwFlags, lpSource, dwMessageId, dwLanguageId,
          (LPWSTR)&lpBuffer, nSize, Args);

  if (buffsize == 0){
    wchar_t* tmp = (wchar_t*)malloc(256);
    wcsncpy (tmp,L"error code: ",sizeof(tmp));
    wchar_t* num = (wchar_t*) malloc(256);
    int size = _snwprintf(num,sizeof (num),L"%d",errnumber);
    wcsncat(tmp,num,sizeof(tmp));
    return tmp;
  }else{
    return  lpBuffer;
  }
}

// have to convert from wide char to regular ascii
char*  wchar_to_char(wchar_t* orig){
  size_t origsize = wcslen(orig)+1;
  size_t convertedChars = 0;
  char * nstring = (char*)malloc(origsize);
  wcstombs_s(&convertedChars, nstring, origsize, orig, _TRUNCATE);
  return nstring;
}
#endif

const char* getErrorString(){
#ifdef WIN32
    int error = WSAGetLastError();
    wchar_t *error_msg = wsaerrorstr(error);
    const char *errstring = wchar_to_char(error_msg);
    return errstring;
#else
   return (strerror(errno));
#endif
}

int main(int argc, char** argv){
  struct addrinfo   hints;
  struct addrinfo * local_addr;
  memset((char*)&hints, 0, sizeof(hints));
  hints.ai_flags  = AI_PASSIVE;
  //hints.ai_family = AF_INET6;
  int rc; 

  char hostname[HOSTNAMESZ];
  memset(hostname,0,HOSTNAMESZ);
  initialize_sockets();
  if (argc==1){
    if((rc=gethostname(hostname,HOSTNAMESZ))!=0){
      printf("Failed to gethostname, error:%s",getErrorString());
    }
  }else{
    strncpy(hostname, argv[1],sizeof(hostname));
  }  

  rc = getaddrinfo(hostname,
                        NULL,
                        &hints,
                        &local_addr);

  if (rc == 0){
    int counter = 1;
    addrinfo* next = local_addr;
    do {
      printf("\tai_flags = %d\n\tai_family = %d\n\tai_socktype = %d\n\tai_protocol = %d\n\tai_addrlen = %d\n\tai_canonname = %s\n\tai_addr = %x\n",
        next->ai_flags,
        next->ai_family,
        next->ai_socktype,
        next->ai_protocol,
        next->ai_addrlen,
        next->ai_canonname,
        (int)next->ai_addr);
        char ipaddr[INET6_ADDRSTRLEN];
        //(struct sockaddr_in *)next->ai_addr;
        if (next->ai_family == AF_INET)
          inet_ntop(next->ai_family, (void*)&((struct sockaddr_in*)next->ai_addr)->sin_addr, ipaddr, sizeof(ipaddr));
        else if (next->ai_family == AF_INET6)
          inet_ntop(next->ai_family, (void*)(&((struct sockaddr_in6*)next->ai_addr)->sin6_addr), ipaddr, sizeof(ipaddr));
        else
          printf ("unrecognized address famiy %d\n", next->ai_family);
        printf("address %d = %s\n", counter++, ipaddr);
        next = next->ai_next;
    }while(next);
  }else{
#ifdef WIN32
    const char* errstring = getErrorString();
#else
    const char* errstring = gai_strerror(rc);
#endif
    printf ("getaddrinfo failed, error : %s\n", errstring);
    free ((void*) errstring);
    errstring = NULL;
  }

  cleanup_sockets();
}

