/* -*- mode: C -*- Time-stamp: "2009-08-20 12:57:46 holzplatten"
   *
   *       File:         shFSO.c
   *       Author:       Pedro J. Ruiz Lopez (holzplatten@es.gnu.org)
   *       Date:         Mon Aug 17 19:57:44 2009
   *
   *       Shell para la asignatura Fundamentos de Sistemas Operativos.
   *
   */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

#include <termios.h>

#define MAXLINEA 1024  /* tamaño maximo linea comandos */
#define MAXARG 256     /* numero maximo argumentos linea comm. */
#define SET 1
#define CLEAR 0




/*
   Definiciones de tipos.
 */
typedef struct _node_t {
  int pid;
  char *name;
  int status;
  int stopped;
  int fg;
  struct termios term_mode;

  struct _node_t *next;
} node_t;

typedef struct _list_t {
  node_t * beg;
  node_t * end;
} list_t;





/*
   Prototipos de funciones de manejo de listas.
 */

void list_init(list_t *);
void list_insert(list_t *, node_t *);
int list_remove(list_t *, node_t *);
int list_remove_pid(list_t * , int);
void list_print();




/*
   Variables globales del shell.
 */
list_t proc_list;
int shell_term;
int shell_pid;

sigset_t block_sigchld;

struct termios shell_tmode;



/*
   Prototipos de función relacionados directamente con el shell.
 */

void show_prompt();
void init_shell();
void set_signals(void (*) ());
void handler_sigchld();
void handler_sigint();

void proc_info(node_t *, int status);
void proc_update(node_t *, int status);

int cmd_cd(int argc, char *argv[]);




/*-
  *      Routine:      proc_info
  *
  *      Purpose:
  *              Muestra información sobre el proceso indicado,
  *              de acuerdo al nuevo status.
  *      Conditions:
  *              El proceso debe existir en la lista de procesos.
  *      Returns:
  *              none
  *
  */
void proc_info(node_t *p, int status)
{
  printf("------------ proc_info\n");
}

/*-
  *      Routine:      proc_update
  *
  *      Purpose:
  *              Actualiza el proceso en la lista de procesos
  *              y lo elimina si es necesario.
  *      Conditions:
  *              El proceso debe existir en la lista de procesos.
  *      Returns:
  *              none
  *
  */
void proc_update(node_t *p, int status)
{
  printf("------------ proc_update\n");
}

/*-
  *      Routine:      set_signals
  *
  *      Purpose:
  *              Establece los manejadores delas señales empleadas
  *              por el shell, excepto SIGINT, que le asigna el
  *              manejador handler_sigint, definido en este mismo
  *              archivo.
  *              
  *      Conditions:
  *              none
  *      Returns:
  *              none
  *
  */
void set_signals(void (*f)())
{
  signal(SIGINT, handler_sigint);
  signal(SIGQUIT, f);
  signal(SIGTSTP, f);
  signal(SIGTTIN, f);
  signal(SIGTTOU, f);
}

/*-
  *      Routine:      handler_sigint
  *
  *      Purpose:
  *              Manejador de la señal SIGINT.
  *      Conditions:
  *              none
  *      Returns:
  *              none
  *
  */
void handler_sigint()
{
  printf("Para salir del shell use el comando \"logout\"\n");
}

/*-
  *      Routine:      handler_sigchld
  *
  *      Purpose:
  *              Manejador de la señal SIGCHLD.
  *      Conditions:
  *              none
  *      Returns:
  *              none
  *
  */
void handler_sigchld()
{
  int pid, status;
  node_t *aux;

  aux = proc_list.beg;
  while (aux)
    {
      if (!aux->fg)
	{
	  pid = waitpid(aux->pid, &status, WUNTRACED|WNOHANG);
	  if (pid == aux->pid)
	    {
	      proc_info(aux, status);
	      proc_update(aux, status);
	    }
	}
    }
  
}

/*-
  *      Routine:      init_shell
  *
  *      Purpose:
  *              Inicializa el shell.
  *      Conditions:
  *              none
  *      Returns:
  *              none
  *
  */
void init_shell()
{
  shell_term = STDIN_FILENO;

  set_signals(SIG_IGN);
  signal(SIGCHLD, handler_sigchld);

  shell_pid = getpid();
  if (setpgid (shell_pid, shell_pid) < 0)
    {
      perror ("setpgid (Shell)");
      exit(1);
    }

  tcsetpgrp(shell_term, shell_pid);
  tcgetattr(shell_term, &shell_tmode);

  list_init(&proc_list);

  sigemptyset(&block_sigchld);
  sigaddset(&block_sigchld, SIGCHLD);
}

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
  int separador;
  
  /* si esta vacio */
  if (narg==0) return 0;

  separador=0;
  for (i=0; i<narg; i++)
    {
      if (!strcmp(argumentos[i], ";"))
	{
	  separador = i;
	  break;
	}
    }

  /* Caso genérico: secuencia de comandos de al menos dos comandos. */
  if (separador)
    {
      free(argumentos[separador]);
      argumentos[separador] = NULL;
      ejecuta_comando(argumentos, separador++);
      return ejecuta_comando(&argumentos[separador], narg-separador);
    }

  /* Caso base: un sólo comando. */

  /* comandos internos */
  if (strcmp(argumentos[0],"logout")==0) return 1;
  if (strcmp(argumentos[0],"help")==0) {help(); return 0;}

  if (strcmp(argumentos[0],"cd") == 0)
    {
      cmd_cd(narg, argumentos);
      return 0;
    }
  
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

  init_shell();

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

/*-
  *      Routine:      cmd_cd
  *
  *      Purpose:
  *              Implementación del cambio de directorio.
  *      Conditions:
  *              none
  *      Returns:
  *              >0 si error.
  *              0 e.o.c.
  *
  */
int cmd_cd(int argc, char *argv[])
{
  char *pwd;

  if (argc > 2)
    return 1;

  if (argc == 1)
    {
      pwd = (char *) get_current_dir_name();

      printf(" Directorio actual = %s\n", pwd);
    }
  else
    {
      if (chdir(argv[1]))
	{
	  perror("cd");
	  return 2;
	}

      pwd = (char *) get_current_dir_name();

      setenv("PWD", pwd, 1);
      printf(" Directorio actual = %s\n", pwd);

    }

  free(pwd);
}



/*
   Definición de funciones de manejo de listas.
 */

/*-
  *      Routine:      list_init
  *
  *      Purpose:
  *              Inicializa una lista.
  *      Conditions:
  *              none
  *      Returns:
  *              none
  *
  */
void list_init(list_t *lst)
{
  lst->beg = lst->end = NULL;
}

/*-
  *      Routine:      list_insert
  *
  *      Purpose:
  *              Inserta un nuevo nodo al final de la lista.
  *      Conditions:
  *              La lista debe estar inicializada.
  *      Returns:
  *              none
  *
  */
void list_insert(list_t * lst, node_t * nod)
{
  nod->next = NULL;

  if (lst->end)
    {
      lst->end->next = nod;
      lst->end = nod;
    }
  else
    {
      lst->beg = nod;
      lst->end = nod;
    }
}

/*-
  *      Routine:      list_remove
  *
  *      Purpose:
  *              Elimina el nodo indicado de la lista.
  *      Conditions:
  *              La lista debe estar inicializada
  *      Returns:
  *              1 si no se encontró el elemento en la lista.
  *              0 e.o.c.
  *
  */
int list_remove(list_t * lst, node_t * nod)
{
  node_t *ant = NULL, *aux = lst->beg;

  while (aux != NULL && aux != nod)
    {
      ant = aux;
      aux = aux->next;
    }

  if (aux == NULL)
    return 1;

  if (ant)
    ant->next = aux->next;
  else
    lst->beg = aux->next;

  if (lst->end == aux)
    lst->end = ant;

  free(nod->name);
  free(nod);

  return 0;
}

/*-
  *      Routine:      list_remove_pid
  *
  *      Purpose:
  *              Elimina el nodo cuyo pid sea el indicado.
  *      Conditions:
  *              La lista debe estar inicializada
  *      Returns:
  *              1 si no se encontró el elemento en la lista.
  *              0 e.o.c.
  *
  */
int list_remove_pid(list_t * lst, int pid)
{
  node_t *ant = NULL, *aux = lst->beg;

  while (aux != NULL && aux->pid != pid)
    {
      ant = aux;
      aux = aux->next;
    }

  if (aux == NULL)
    return 1;

  if (ant)
    ant->next = aux->next;
  else
    lst->beg = aux->next;

  if (lst->end == aux)
    lst->end = ant;

  free(aux->name);
  free(aux);

  return 0;
}
