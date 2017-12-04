#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <curses.h>




/*
    CONCURRENT SERVER: THREAD EXAMPLE
    Must be linked with the "pthread" library also, e.g.:
       cc -o example example.c -lnsl -lsocket -lpthread 

    This program creates a connection socket, binds a name to it, then
    listens for connections to the sockect.  When a connection is made,
    it accepts messages from the socket until eof, and then waits for
    another connection...

    This is an example of a CONCURRENT server -- by creating threads several
    clients can be served at the same time...

    This program has to be killed to terminate, or alternately it will abort in
    120 seconds on an alarm...
*/

#define PORTNUMBER 10015


struct serverParm {
           int connectionDesc;
};



   

void *serverThread(void *parmPtr) {

    #define PARMPTR ((struct serverParm *) parmPtr)
    int recievedMsgLen;
    char messageBuf[1025];
	sqlite3 *db;
    char *zErrMsg = 0;
	sqlite3_stmt *stmt;
    int rc;
    char *sql;
    const char* data = "";
    FILE *fptr;
	
	
	
 
 
   

    /* Server thread code to deal with message processing */
    printf("DEBUG: connection made, connectionDesc=%d\n",
            PARMPTR->connectionDesc);
    if (PARMPTR->connectionDesc < 0) {
        printf("Accept failed\n");
        return(0);    /* Exit thread */
    }
    
    /* Receive messages from sender... */
    while ((recievedMsgLen=
            read(PARMPTR->connectionDesc,messageBuf,sizeof(messageBuf)-1)) > 0) 
    {
        recievedMsgLen[messageBuf] = '\0';
        printf("Message: %s\n",messageBuf);
		
		//Open the database first
		
		 /* Open database */
		rc = sqlite3_open("emp.db", &db);

		if( rc ) {
			fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
			return(0);
		} else {
			fprintf(stderr, "Opened database successfully\n");
		}
		
		/* Create SQL statement */
		sql = messageBuf;
		printf("SQl Statement is : %s\n",sql);
		
		if (write(PARMPTR->connectionDesc,sql,200) < 0) {
               perror("Server: write error");
               return(0);
        }
	 
		 
		 
   	sqlite3_stmt *stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  
    int rowCount = 0;
    rc = sqlite3_step(stmt);
    char buf[65535];
  
    while (rc != SQLITE_DONE && rc != SQLITE_OK)
    {
        rowCount++;
        int colCount = sqlite3_column_count(stmt);
        for (int colIndex = 0; colIndex < colCount; colIndex++)
        {
            const char * columnName = sqlite3_column_name(stmt, colIndex);
            const unsigned char * valChar = sqlite3_column_text(stmt, colIndex);
            printf(" %s", valChar);
            strcat(buf, " ");            
            strcat(buf, valChar);                       
        }        
        printf("\n");
        strcat(buf, "\n");  
         
        rc = sqlite3_step(stmt);
   	}
    rc = sqlite3_finalize(stmt);
		/*  open for writing */	
      fptr = fopen("a4p1ServerLog.txt", "w");
      
       if (fptr == NULL)
      {
        printf("File does not exists \n");
        return;
      }else{
		
		   printf("File created and open for writing \n");
		
	    }
		
			printf("Writing data to the file %s\n", buf);
      fprintf(fptr, "%s", buf);   //Write data to the file
		   //printf("ALL DONE - RESULT IS  --------------> \n %s\n", buf);
       // write(PARMPTR->connectionDesc,buf,65535);
       //write(PARMPTR->connectionDesc, buf, strlen(buf));
       //send(PARMPTR->connectionDesc,buf,strlen(buf),0);
   
   	//printf("Are you live  --------------> \n %s\n", PARMPTR->connectionDesc); 
		if (write(PARMPTR->connectionDesc,buf,strlen(buf)) < 0) {
     perror("Server: write error");
      return(0);
   }
		fprintf(stdout, "Operation done successfully\n");		
		sqlite3_close(db);
	
    }
    close(PARMPTR->connectionDesc);  /* Avoid descriptor leaks */
    free(PARMPTR);                   /* And memory leaks */
    fclose(fptr);//Close the file
    return(0);                       /* Exit thread */
}

main () {
    int listenDesc;
    struct sockaddr_in myAddr;
    struct serverParm *parmPtr;
    int connectionDesc;
    pthread_t threadID;

    /* For testing purposes, make sure process will terminate eventually */
    alarm(1200);  /* Terminate in 120 seconds */

    /* Create socket from which to read */
    if ((listenDesc = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("open error on socket");
        exit(1);
    }

	
    /* Create "name" of socket */
    myAddr.sin_family = AF_INET;
    myAddr.sin_addr.s_addr = INADDR_ANY;
    myAddr.sin_port = htons(PORTNUMBER);
        
    if (bind(listenDesc, (struct sockaddr *) &myAddr, sizeof(myAddr)) < 0) {
        perror("bind error");
        exit(1);
    }

    /* Start accepting connections.... */
    /* Up to 5 requests for connections can be queued... */
    listen(listenDesc,5);

    while (1) /* Do forever */ {
        /* Wait for a client connection */
        connectionDesc = accept(listenDesc, NULL, NULL);

        /* Create a thread to actually handle this client */
        parmPtr = (struct serverParm *)malloc(sizeof(struct serverParm));
        parmPtr->connectionDesc = connectionDesc;
        if (pthread_create(&threadID, NULL, serverThread, (void *)parmPtr) 
              != 0) {
            perror("Thread create error");
            close(connectionDesc);
            close(listenDesc);
            exit(1);
        }
        printf("Parent ready for another connection\n");
	}
}
