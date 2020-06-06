/* DMUTEX (2009) Sistemas Operativos Distribuidos
 * C�digo de Apoyo
 *
 * ESTE C�DIGO DEBE COMPLETARLO EL ALUMNO:
 *    - Para desarrollar las funciones de mensajes, reloj y
 *      gesti�n del bucle de tareas se recomienda la implementaci�n
 *      de las mismas en diferentes ficheros.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_PROCS 10
enum
{
    MSG,
    LOCK,
    OK
};

int max(int a, int b)
{
    if (a > b)
    {
        return a;
    }
    return b;
}

struct miMensaje
{
    int tipo;
    int LC[MAX_PROCS];
    char seccion[80];
};

int puerto_udp;

int main(int argc, char *argv[])
{
    int port;
    char line[80], proc[80];
    int scket;
    struct sockaddr_in midireccion, direcciondestino;
    typedef struct procesos
    {
        char nombre[10];
        int puerto;
    } procesos;
    struct procesos process[MAX_PROCS];
    struct miMensaje msj;

    if (argc < 2)
    {
        fprintf(stderr, "Uso: proceso <ID>\n");
        return 1;
    }

    /* Establece el modo buffer de entrada/salida a l�nea */
    setvbuf(stdout, (char *)malloc(sizeof(char) * 80), _IOLBF, 80);
    setvbuf(stdin, (char *)malloc(sizeof(char) * 80), _IOLBF, 80);

    if ((scket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Error creando socket");
        return 1;
    }
    midireccion.sin_family = AF_INET;
    midireccion.sin_addr.s_addr = INADDR_ANY;
    midireccion.sin_port = 0;
    if (bind(scket, (struct sockaddr *)&midireccion, sizeof(midireccion)) < 0)
    {
        perror("error en bind");
        close(scket);
        return 1;
    }

    socklen_t addrlen = sizeof(midireccion);

    if (getsockname(scket, (struct sockaddr *)&midireccion, (socklen_t *)&addrlen) < 0)
    {
        perror("error en getsockname");
        return 1;
    }

    puerto_udp = midireccion.sin_port; /* Se determina el puerto UDP que corresponda.
                      Dicho puerto debe estar libre y quedar�
                      reservado por dicho proceso. */

    fprintf(stdout, "%s: %d\n", argv[1], puerto_udp);

    int nprocess = 0;
    int position = 0;

    for (; fgets(line, 80, stdin);)
    {
        if (!strcmp(line, "START\n")) //START
            break;

        sscanf(line, "%[^:]: %d", proc, &port);
        /* Habra que guardarlo en algun sitio */
        strcpy(process[nprocess].nombre, proc);
        process[nprocess].puerto = port;

        if (!strcmp(proc, argv[1]))
        {
            /* Este proceso soy yo */
            position = nprocess;
        }
        nprocess++;
    }

    /* Inicializar Reloj */
    int LC[nprocess];
    //int relojLock[nprocess];
    int i;
    for (i = 0; i < nprocess; i++)
    {
        LC[i] = 0;
    }

    /* Procesar Acciones */

    char accion[20], parametroAccion[60];

    for (; fgets(line, 80, stdin);)
    {
        sscanf(line, "%s %s", accion, parametroAccion);
        //fprintf(stderr, "\n\n LC%s[%d,%d,%d] \n\n", process[position].nombre, LC[0], LC[1], LC[2]);

        if (!strcmp(accion, "EVENT"))
        {
            LC[position]++;
            printf("%s: TICK\n", process[position].nombre);
        }
        else if (!strcmp(accion, "GETCLOCK"))
        {
            printf("%s: LC[", process[position].nombre);
            int t;
            for (t = 0; t < nprocess - 1; t++)
            {
                printf("%d,", LC[t]);
            }
            printf("%d]\n", LC[nprocess - 1]);
        }
        else if (!strcmp(accion, "MESSAGETO"))
        {
            LC[position]++;
            printf("%s: TICK\n", process[position].nombre);
            direcciondestino.sin_family = AF_INET;
            direcciondestino.sin_addr.s_addr = INADDR_ANY;
            int puerto;
            for (i = 0; i < nprocess; i++)
            {
                if (strcmp(process[i].nombre, parametroAccion) == 0)
                {
                    puerto = i;
                }
            }
            direcciondestino.sin_port = process[puerto].puerto;
            msj.tipo = MSG;
            memcpy(&msj.LC, &LC, sizeof(int) * nprocess);
            strcpy(msj.seccion, process[position].nombre);
            if (sendto(scket, &msj, sizeof(msj), 0, (struct sockaddr *)&direcciondestino, sizeof(direcciondestino)) < 0)
            {
                perror("error en sendto");
                close(scket);
                return 1;
            }
            printf("%s: SEND(MSG,%s)\n", process[position].nombre, parametroAccion);
        }
        else if (!strcmp(accion, "RECEIVE"))
        {
            if (read(scket, &msj, sizeof(msj)) < 0)
            {
                perror("error en read");
                close(scket);
                return 1;
            }
            switch (msj.tipo)
            {
            case 0: //MSG
                printf("%s: RECEIVE(MSG,%s)\n", process[position].nombre, msj.seccion);
                //LC[position]++;
                printf("%s: TICK\n", process[position].nombre);
                for (i = 0; i < nprocess; i++)
                    {
                        int maximo = max(msj.LC[i], LC[i]);
                        if (position == i)
                        {
                            maximo++;
                        }
                        LC[i] = maximo;
                    }
                LC[position] = max(msj.LC[position], LC[position]);
                break;
            case 1: //LOCK
                printf("%s: RECEIVE(LOCK,%s)\n", process[position].nombre, msj.seccion);
                //LC[position]++;
                printf("%s: TICK\n", process[position].nombre);
                break;
            case 2: //OK
                fprintf(stderr, "\n\n OK \n\n");
                break;
            default:
                break;
            }

        }
        else if (!strcmp(line, "FINISH\n"))
        {
            printf("FINISH[%d]", process[position].puerto);
            exit(0);
        }
        else if (!strcmp(accion, "LOCK")) {
            LC[position]++;
            
            direcciondestino.sin_family = AF_INET;
            direcciondestino.sin_addr.s_addr = INADDR_ANY;
            for (i = 0; i < nprocess; i++)
            {
                if (i != position)
                {
                    printf("%s: TICK", process[position].nombre);
                    direcciondestino.sin_port = process[i].puerto;
                    msj.tipo = LOCK;
                    memcpy(&msj.LC, &LC, sizeof(int) * nprocess);
                    strcpy(msj.seccion, process[position].nombre);
                    if (sendto(scket, &msj, sizeof(msj), 0, (struct sockaddr *)&direcciondestino, sizeof(direcciondestino)) < 0)
                    {
                        perror("error en sendto");
                        close(scket);
                        return 1;
                    }
                    printf("%s: SEND(LOCK,%s)\n", process[position].nombre, process[i].nombre);
                }
            }
            
        }
        else
        {
            exit(0);
        }
    }

    return 0;
}