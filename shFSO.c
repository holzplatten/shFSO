/* -*- mode: C -*- Time-stamp: "2013-09-01 15:12:24 holzplatten"
   *
   *       File:         shFSO.c
   *       Author:       Pedro J. Ruiz Lopez (holzplatten@es.gnu.org)
   *       Date:         Mon Aug 17 19:57:44 2009
   *
   *       Shell para la asignatura Fundamentos de Sistemas Operativos.
   *
   */

/*
  Copyright (C) 2009, 2013 Pedro J. Ruiz López <holzplatten@es.gnu.org>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
int list_index(list_t *, node_t *);
node_t * list_elem(list_t *, int);
int list_length(list_t *);



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
void cmd_jobs(int argc, char *argv[]);
void cmd_fg(int argc, char *argv[]);
void cmd_bg(int argc, char *argv[]);

int is_nat(char *);




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
  if (WIFSTOPPED(status))
    {
      printf(" [%d] %s (pid=%d) : PARADO\n",
	     list_index(&proc_list, p), p->name, p->pid);
    }
  else
    {
      printf(" [%d] %s (pid=%d) : TERMINADO\n",
	     list_index(&proc_list, p), p->name, p->pid);
    }
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
  if (WIFSTOPPED(status))
    {
      p->stopped = SET;
    }
  else
    {
      list_remove(&proc_list, p);
    }
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
  node_t *aux, *next;

  sigprocmask(SIG_BLOCK, &block_sigchld, NULL);
  aux = proc_list.beg;
  while (aux)
    {
      next = aux->next;

      pid = waitpid(aux->pid, &status, WUNTRACED|WNOHANG);
      if (pid == aux->pid)
	{
	  proc_info(aux, status);
	  proc_update(aux, status);
	}

      aux = next;
    }
  sigprocmask(SIG_UNBLOCK, &block_sigchld, NULL);
  
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
  printf("   Shell para FSO 2009: hsh\n");
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
  int strip_pos, bg=0;
  node_t *new_proc;
  
  /* si esta vacio */
  if (narg==0) return 0;

  strip_pos=0;
  for (i=0; i<narg; i++)
    {
      if (!strcmp(argumentos[i], ";"))
	{
	  strip_pos = i;
	  break;
	}
    }

  /* Caso genérico: secuencia de comandos de al menos dos comandos. */
  if (strip_pos)
    {
      free(argumentos[strip_pos]);
      argumentos[strip_pos] = NULL;
      ejecuta_comando(argumentos, strip_pos++);
      return ejecuta_comando(&argumentos[strip_pos], narg-strip_pos);
    }

  /* Caso base: un sólo comando. */

  if (strcmp(argumentos[narg-1],"&")==0)
    {
      bg=1;
      free(argumentos[narg-1]);
      argumentos[narg-1]=NULL;
      narg--;
    }

  /* comandos internos */
  if (strcmp(argumentos[0],"logout")==0) return 1;
  if (strcmp(argumentos[0],"help")==0) {help(); return 0;}

  if (strcmp(argumentos[0], "cd") == 0)
    {
      cmd_cd(narg, argumentos);
      return 0;
    }

  if (strcmp(argumentos[0], "jobs") == 0)
    {
      cmd_jobs(narg, argumentos);
      return 0;
    }

  if (strcmp(argumentos[0], "fg") == 0)
    {
      cmd_fg(narg, argumentos);
      return 0;
    }

  if (strcmp(argumentos[0], "bg") == 0)
    {
      cmd_bg(narg, argumentos);
      return 0;
    }
  
  /* Debido a que no podemos predecir el comportamiento del scheduler
     del SO en todo momento, tampoco podemos predecir que las rutinas
     siguientes se ejecuten antes que el proceso hijo resultante del
     fork. Por ello es conveniente bloquear la señal SIGCHLD antes de
     "forkear", y de paso ahorrarme otro quebradero de cabeza más. :)
  */
  sigprocmask(SIG_BLOCK, &block_sigchld, NULL);

  pid=fork();
  switch(pid)
    {
    case -1:
      /* Error en la llamada a fork!
	 Problema grave: terminar devolviendo un valor no-cero.
      */
      perror("fork");
      exit(1);
      break;

    case 0:
      /**** Hijo ****/
      pid = getpid();
      setpgid(pid, pid);

      if (!bg)
	tcsetpgrp(shell_term, pid);

      set_signals(SIG_DFL);

      execvp(argumentos[0], argumentos);

      /* Este código no se debería ejecutar nunca a menos que falle
	 la llamada a execvp, lo cual consideramos un error
	 irrecuperable: terminar devolviendo un valor no-cero. 
      */
      perror("execvp");
      exit(1);
      break;

    default:
      /**** Padre ****/

      if (bg)
	printf("Ejecutando... [%d] %s (pid=%d) ... en segundo plano\n",
	       list_length(&proc_list)+1, argumentos[0], pid);

      /* Salvar el modo actual del terminal. */
      tcgetattr(shell_term, &shell_tmode);

      new_proc = malloc(sizeof(node_t));

      new_proc->pid = pid;
      new_proc->name = strdup(argumentos[0]);
      new_proc->term_mode = shell_tmode;
      new_proc->fg = !bg;
      new_proc->stopped = CLEAR;

      list_insert(&proc_list, new_proc);

      /* Cada proceso en su propio grupo. */
      setpgid(pid,pid);

      if (!bg)
	{
	  /* Dar el terminal al hijo. */
	  tcsetpgrp(shell_term, pid);

	  waitpid(pid, &status, WUNTRACED);

	  new_proc->status = status;

	  /* Salvar el modo del terminal empleado por el hijo. */
	  tcgetattr(shell_term, &new_proc->term_mode);

	  /* Recuperar el terminal. */
	  if (tcsetpgrp(shell_term, shell_pid) == -1)
	    {
	      perror("tcsetpgrp");
	      exit(1);
	    }

	  /* Restaurar el modo del terminal para el shell. */
	  tcsetattr(shell_term, TCSANOW, &shell_tmode);

	  if (WIFSTOPPED(status))
	    proc_info(new_proc, status);
	  proc_update(new_proc, status);
	}

    }

  sigprocmask(SIG_UNBLOCK, &block_sigchld, NULL);

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
  *      Routine:      cmd_jobs
  *
  *      Purpose:
  *              Implementación del comando interno 'jobs'.
  *              Muestra en pantalla el estado de la lista de trabajos.
  *      Conditions:
  *              La lista de trabajos debe ser válida.
  *      Returns:
  *              none
  *
  */
void cmd_jobs(int argc, char *argv[])
{
  int i, lng;
  node_t *p;

  if (argc != 1)
    {
      printf("ERROR: Parámetro no válido\n");
      printf(" Sintaxis: jobs\n");
      return;
    }

  lng = list_length(&proc_list);
  if (lng == 0)
    {
      printf(" La lista de trabajos está vacía.\n");
    }
  else
    {
      for (i=1, p=proc_list.beg;
	   i<=lng;
	   i++, p=p->next)
	{
	  printf(" [%d] %s (pid=%d) : %s\n",
		 i, p->name, p->pid, p->stopped ? "PARADO" : "EN EJECUCIÓN");

	}
    }
    
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

/*-
  *      Routine:      cmd_fg
  *
  *      Purpose:
  *              Pone en primer plano un proceso que se encuentra
  *              parado o en segundo plano.
  *      Conditions:
  *              none
  *      Returns:
  *              none
  *
  */
void cmd_fg(int argc, char *argv[])
{
  int n;
  node_t *p;

  if (argc != 2 || !is_nat(argv[1]))
    {
      printf(" Sintaxis: fg numero_de_trabajo\n");
      return;
    }

  sigprocmask(SIG_BLOCK, &block_sigchld, NULL);
  p = list_elem(&proc_list, atoi(argv[1]) );
  if (!p)
    {
      printf("ERROR: número de trabajo no válido\n");

      sigprocmask(SIG_UNBLOCK, &block_sigchld, NULL);
      return;
    }

  /* Salvar el modo actual del terminal. */
  tcgetattr(shell_term, &shell_tmode);

  /* Restaurar el modo del terminal tal como lo dejó el hijo. */
  tcsetattr(shell_term, TCSANOW, &p->term_mode);
  /* Dar el terminal al hijo. */
  tcsetpgrp(shell_term, p->pid);

  p->fg = SET;
  p->stopped = CLEAR;

  printf("Pasando... [%d] %s (pid=%d) ... a primer plano\n",
	 list_index(&proc_list, p), p->name, p->pid);

  kill(p->pid, SIGCONT);

  waitpid(p->pid, &p->status, WUNTRACED);

  /* Salvar el modo del terminal empleado por el hijo. */
  tcgetattr(shell_term, &p->term_mode);

  /* Recuperar el terminal. */
  if (tcsetpgrp(shell_term, shell_pid) == -1)
    {
      perror("tcsetpgrp");
      exit(1);
    }

  /* Restaurar el modo del terminal para el shell. */
  tcsetattr(shell_term, TCSANOW, &shell_tmode);

  proc_info(p, p->status);
  proc_update(p, p->status);

  sigprocmask(SIG_UNBLOCK, &block_sigchld, NULL);

}

/*-
  *      Routine:      cmd_bg
  *
  *      Purpose:
  *              Pone en ejecución en segundo plano un proceso
  *              que se encuentra parado.
  *      Conditions:
  *              none
  *      Returns:
  *              none
  *
  */
void cmd_bg(int argc, char *argv[])
{
  int n;
  node_t *p;

  if (argc != 2 || !is_nat(argv[1]))
    {
      printf(" Sintaxis: bg numero_de_trabajo\n");
      return;
    }

  sigprocmask(SIG_BLOCK, &block_sigchld, NULL);
  p = list_elem(&proc_list, atoi(argv[1]) );
  if (!p)
    {
      printf("ERROR: número de trabajo no válido\n");

      sigprocmask(SIG_UNBLOCK, &block_sigchld, NULL);
      return;
    }

  if (!p->stopped)
    {
      printf("ERROR: El trabajo ya se encuentra en segundo plano\n");
      sigprocmask(SIG_UNBLOCK, &block_sigchld, NULL);
      return;
    }

  p->fg = CLEAR;
  p->stopped = CLEAR;

  printf("Pasando... [%d] %s (pid=%d) ... a segundo plano\n",
	 list_index(&proc_list, p), p->name, p->pid);

  kill(p->pid, SIGCONT);

  sigprocmask(SIG_UNBLOCK, &block_sigchld, NULL);
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

/*-
  *      Routine:      list_index
  *
  *      Purpose:
  *              Obtiene la posición en la lista del proceso dado.
  *      Conditions:
  *              La lista debe estar inicializada.
  *      Returns:
  *              Un entero con la posición del proceso o 0 si
  *              éste no se encontró.
  *
  */
int list_index(list_t *lst, node_t *p)
{
  int i=1;
  node_t *aux=lst->beg;

  while (aux && aux!=p)
    {
      i++;
      aux = aux->next;
    }

  if (aux==NULL)
    return 1;

  return i;
}

/*-
  *      Routine:      list_length
  *
  *      Purpose:
  *              Calcula la longitud de una lista.
  *      Conditions:
  *              La lista debe ser válida.
  *      Returns:
  *              Un entero.
  *
  */
int list_length(list_t *lst)
{
  int i=0;
  node_t *aux=lst->beg;

  while (aux!=NULL)
    {
      i++;
      aux=aux->next;
    }

  return i;
}

/*-
  *      Routine:      list_elem
  *
  *      Purpose:
  *              Obtiene el trabajo que ocupa la posición 'pos'
  *              en la lista de trabajos.
  *      Conditions:
  *              La lista debe ser válida.
  *      Returns:
  *              Un apuntador a la estructura-nodo.
  *              NULL si no se encontró el trabajo.
  *
  */
node_t * list_elem(list_t *lst, int pos)
{
  node_t *aux=lst->beg;

  if (pos==0)
    return NULL;

  while (aux && --pos)
    aux = aux->next;

  return aux;
}


/*
   Definición de funciones de apoyo.
 */

/*-
  *      Routine:      is_nat
  *
  *      Purpose:
  *              Decide si una cadena de caracteres representa
  *              un número natural.
  *      Conditions:
  *              Se debe proporcionar un apuntador a una cadena
  *              de caracteres válida
  *      Returns:
  *              1 <-> Sí
  *              0 <-> No
  *
  */
int is_nat(char *c)
{
  while (*c)
    {
      if (!isdigit(*c))
	return 0;
      c++;
    }

  return 1;
}
