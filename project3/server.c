#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "base.h"
#include "cgi_response.h"
#include "server.h"

#define CRLF "\r\n"

void
serve_request (int connfd)
{
  char *method = NULL;
  char *uri = NULL;
  char *query = NULL;
  char *version = NULL;
  char *boundary = NULL;
  char *body = NULL;
  ssize_t size = retrieve_request (connfd, &method, &uri, &query, &version,
                                  &boundary, &body);
  if (size < 0)
    {
      // Ignore certain types of request (such as favicon.ico)
      shutdown (connfd, SHUT_RDWR);
      close (connfd);
      return;
    }

  // The following lines are useful for observing what request you get.
  // Specifically, consider the following HTTP request:
  //    GET /cgi-bin/show.cgi?db=hashes.txt&all=true HTTP/1.1\r\n
  //    Hostname: localhost\r\n
  //    Connection: close\r\n
  //    \r\n
  // The variables would have the following values:
  //    method = "GET"                    [dynamically allocated, needs freed]
  //    uri = "cgi-bin/show.cgi"          [dynamically allocated, needs freed]
  //    query = "db=hashes.txt&all=true"  [dynamically allocated, needs freed]
  //    size = 0                        [only meaningful in POST file uploads]
  //    boundary = NULL                 [only meaningful in POST file uploads]
  //    body = NULL                     [only meaningful in POST file uploads]
  printf ("+++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  printf ("METHOD: %s\n", method);
  printf ("URI: %s\n", uri);
  printf ("QUERY: %s\n", query);
  printf ("VERSION: %s\n", version);
  printf ("SIZE: %zd\n", size);
  printf ("BOUNDARY: %s\n", boundary);
  printf ("BODY [each line ends in \\r\\n]:\n%s\n", body);
  printf ("+++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

  // TODO [PART]: Replace this hard-coded HTTP response with the contents
  // of an HTML file (if the URI begins with "srv_root") or with the output
  // of executing a CGI program (if the URI begins with "cgi-bin"). For CGI
  // files, you will need to pipe/fork/dup2/exec the program. Also, note that
  // the query string will need to be passed using an environment variable
  // called "QUERY_STRING".
  
  //added by Chris
  if(strstr(uri,"srv_root") != NULL)
  {
    //replace with contents of HTML file
    char *response = html_response (uri, version);
    write (connfd, response, strlen (response));
  }    
  
  else if(strstr(uri,"cgi-bin") != NULL)
  {
    int fd[2];
    pipe(fd);
    pid_t child = fork();
    if(child == 0)
    {
      dup2(fd[1],STDOUT_FILENO);
      execlp(uri,uri,NULL);
    }
    close(fd[1]);
    char program_buffer[1024];
    char* entire_file = strdup("");
    int bytes_read;
    while((bytes_read = read(fd[0],program_buffer,1023)) > 0)
    {
      program_buffer[bytes_read] = '\0';
      entire_file = realloc(entire_file,strlen(entire_file)+strlen(program_buffer)+1);
      strcat(entire_file,program_buffer);    
    }
    // write headers before writing entire file
    // the process of building a header should be in it's own function.
    size_t filesize = strlen(entire_file);
    char *header = malloc (strlen (version) + 1);
    strcpy (header, version);

    char *headers = " 200 OK\r\n"
                    "Content-Type: text/html; charset=UTF-8\r\n"
                    "Content-Length: ";
    // Create a 21-character buffer to store the file size as a string (use
    // snprintf() to convert the size_t into a char*). We can safely assume the
    // string version while fit, as size_t is a 64-bit value that has a maximum
    // value of 18,446,744,073,709,551,615.
    char size_as_string[21];
    char *One_point_one_message = "Connection: close\r\n\r\n";

    if (strcmp (version, "HTTP/1.0") == 0)
      {
        memset (size_as_string, 0, 21);
        sprintf (size_as_string, "%ld", filesize);
        header = realloc (header, strlen (header) + strlen (headers)
                                      + strlen (size_as_string) + 5);
        strcat (header, headers);
        strcat (header, size_as_string);
        strcat (header, "\r\n\r\n");
      }

    else if (strcmp (version, "HTTP/1.1") == 0)
      {
        memset (size_as_string, 0, 21);
        snprintf (size_as_string, sizeof (filesize), "%ld", filesize);
        header = realloc (header, strlen (header) + strlen (headers)
                                      + strlen (size_as_string)
                                      + strlen (One_point_one_message));
        strcat (header, headers);
        strcat (header, size_as_string);
        strcat (header, "\r\n");
        strcat (header, "Connection: close\r\n\r\n");
      }
    write(connfd,header,strlen(header)); // write the header
    write(connfd,entire_file,strlen(entire_file)); // write the entire file
  }
  //added by Chris
  
  shutdown (connfd, SHUT_RDWR);
  close (connfd);

  // Clean up variables and close the connection
  if (method != NULL)
    free (method);
  if (query != NULL)
    free (query);
  if (boundary != NULL)
    free (boundary);
  if (body != NULL)
    free (body);

  // TODO [PART]: If the URI is for the shutdown.cgi file, kill the current
  // process with the SIGUSR1 signal.
  if (uri != NULL)
    free (uri);

  return;
}
