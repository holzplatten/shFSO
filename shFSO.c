/* -*- mode: C -*- Time-stamp: "2009-08-17 20:08:01 holzplatten"
   *
   *       File:         shFSO.c
   *       Author:       Pedro J. Ruiz Lopez (holzplatten@es.gnu.org)
   *       Date:         Mon Aug 17 19:57:44 2009
   *
   *       Shell para la asignatura Fundamentos de Sistemas Operativos
   *
   */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

#define MAXLINEA 1024  /* tamaño maximo linea comandos */
#define MAXARG 256     /* numero maximo argumentos linea comm. */
#define SET 1
#define CLEAR 0




void show_prompt();




/*-
  *      Routine:      show_prompt
  *
  *      Purpose:
  *              Muestra un prompt con el CWD
  *      Conditions:
  *              none
  *      Returns:
  *              none
  *
  */
void show_prompt()
{
  char *pwd = (char *) get_current_dir_name();

  printf("%s> ", pwd);
  free(pwd);
}


/*============================================================*/    
/* muestra la ayuda del shell */
int help()
{
  printf("   ---------------------\n");
  printf("   Shell basico FSO 2007\n");
  printf("   ---------------------\n");
  printf("   help                   esta ayuda\n");
  printf("   logout                 salir \n");
  printf("   programa <args>        ejecuta el programa \n");
  printf("\n");
}

/*============================================================*/    
/* lee linea de comandos de la entrada estandar */
int nueva_linea(char * linea, int len)
{
	int i=0;
	while(i<len)
	{
		if((linea[i]=(char)fgetc(stdin))==-1)
		{
			perror("fgetc");
			exit(-1);
		}
		
		if (linea[i]=='\n')
		{
			linea[i]='\0';
			return(i);
		}
		i++;
		if(i==len) linea[i]='\0';
	}
	return (i);
}

/*============================================================*/    
/* analiza una linea y la trocea en argumentos reservando memoria
   y colocandolos en la lista de punteros a char "argumentos"
   y terminadolo en NULL. Devuelve el numero de argumentos leidos */
int lee_linea(char * linea, char **argumentos)
{
  char *token;
  int narg=0;

  if((token=strtok(linea," "))!=NULL) /* busca una palabra separada por espacios */
    {
      argumentos[narg]=(char *)strdup(token);/* copia la palabra  */
      narg++; /* incrementa el contador de palabras */
      while ((token=strtok(NULL," "))!=NULL) /* busca mas palabras en la misma linea */
	{
	  argumentos[narg]=(char *)strdup(token);
	  narg++;
	} 
    }
  argumentos[narg]=NULL;
  return narg;
}
  	
    
/*============================================================*/    
/* libera la memoria de los argumentos */
int libera_mem_arg(char ** argumentos, int n_arg)
{
  int i;
  for (i=0;i<n_arg;i++) free(argumentos[i]);
  return;
}



/*============================================================*/    
/* ejecuta el comando formado la lista "argumentos" con 
   un numero de palabras "narg". Devuelve 1 en el caso
   de ejecutar el commando de finalizacion (logout) */
int ejecuta_comando(char ** argumentos, int narg)
{
  int i, pid;
  int status;
  
  /* si esta vacio */
  if (narg==0) return 0;

  /* comandos internos */
  if(strcmp(argumentos[0],"logout")==0) return 1;
  if(strcmp(argumentos[0],"help")==0) {help(); return 0;}
  
  /* ejecuta comando externo */
  if ((pid=fork())==0) /* lanza la ejecucion de un proceso hijo */
    { /* hijo */
      execvp(argumentos[0],argumentos);
      printf("Error, no se puede ejecutar: %s\n",argumentos[0]);
      exit(-1); 
    }
  /* padre espera a la conclusion del comando - proceso hijo */
  waitpid(pid,&status,0);
  return 0;
}

/*============================================================*/    
/* funcion principal */ 
/*============================================================*/    
main()
{
  char *argumentos[MAXARG];
  int narg, i, j, status;
  int fin=0;
  char linea[MAXLINEA];

  help(); /* muestra aviso y ayuda */
  while(!fin)  
    {
      /* escribe el prompt */
      show_prompt();
      /* lee la linea de comandos*/
      nueva_linea(linea,MAXLINEA);
      /* analiza linea de comandos y la separa en argumentos */  
      narg=lee_linea(linea,argumentos);
      /* ejecuta linea comandos */
      fin=ejecuta_comando(argumentos,narg);
      /* libera memoria de los argumentos */
      libera_mem_arg(argumentos, narg);
    }
  printf("Bye\n");
  exit(0);
} 
