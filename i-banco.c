/*
// Projeto SO - exercicio 1, version 1
// Sistemas Operativos, DEI/IST/ULisboa 2016-17
*/

#include "commandlinereader.h"									//TRANSFERIR A TRANSF DA DESTINO PARA A ORIGEM
#include "contas.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>


#define COMANDO_DEBITAR "debitar"
#define COMANDO_CREDITAR "creditar"
#define COMANDO_LER_SALDO "lerSaldo"
#define COMANDO_SIMULAR "simular"
#define COMANDO_SAIR "sair"
#define COMANDO_TRANSFERIR "transferir"
#define NUM_TRABALHADORAS 3
#define CMD_BUFFER_DIM 6

#define MAXARGS 10
#define BUFFER_SIZE 100

typedef struct {	
						//Estrutura que guarda o input do utilizador numa estrutura, com operacao, idConta e valor, se for aplicavel
    	int operacao;
    	int idConta;
    	int idContaDestino;
    	int valor;

    } comando_t;

comando_t criaComando (int op, int idConta, int idContaDestino, int valor){					//Funcao auxiliar que gera novas estruturas do tipo comando_t

	comando_t opTemp;					
	opTemp.operacao = op;
	opTemp.idConta = idConta;
	opTemp.idContaDestino = idContaDestino;
	opTemp.valor = valor;
	return opTemp;
}


sem_t sem_produtores, sem_consumidores;									//Definicao das diversas variaveis, semaforos, mutex e do buffer responsavel por
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;						
pthread_cond_t varcondicao;
pthread_mutex_t mutexvarcond = PTHREAD_MUTEX_INITIALIZER;
int buff_write_idx = 0, buff_read_idx = 0, Ncomandos = 0, Var_Sair = 0; 
comando_t cmd_buffer[CMD_BUFFER_DIM];
FILE *trab;


void cond_signal(pthread_cond_t * cond)	{						//guardar todas as estruturas comando_t geradas
if (pthread_cond_signal(cond) != 0) {
	perror("Erro no signal da variavel de condicao");
	}
}

void* executa_operacao() {

	while(1){									//Ciclo while infinito para as threads nunca fecharem

		sem_wait(&sem_consumidores);

		if(Var_Sair == 1 && Ncomandos == 0){	//Condicao responsavel por verificar se o comando sair foi chamado e se ainda ha comandos no buffer.Caso nao
			pthread_exit(NULL);					//existam mais comandos no buffer as thread sao encerradas.
		}	

		int TID = pthread_self();

		pthread_mutex_lock(&lock);				//Vamos entrar numa seccao critica pelo que temos de fechar o trinco de modo a proteger as variaveis partilhadas

		comando_t temporario  = cmd_buffer[buff_read_idx];
		buff_read_idx = (buff_read_idx+1) % (CMD_BUFFER_DIM);

		if(temporario.operacao == 1) {

			if(debitar(temporario.idConta, temporario.valor) < 0){
				printf("%s(%d, %d): Erro\n\n", COMANDO_DEBITAR, temporario.idConta, temporario.valor);
				fprintf(trab, "%d:%s(%d, %d): Erro\n", TID, COMANDO_DEBITAR, temporario.idConta, temporario.valor);
				pthread_mutex_lock(&mutexvarcond);
				if (Ncomandos-- == 0) cond_signal(&varcondicao);
				pthread_mutex_unlock(&mutexvarcond);
			}
			else {
				printf("%s(%d, %d): OK\n\n", COMANDO_DEBITAR, temporario.idConta, temporario.valor);
				fprintf(trab, "%d:%s(%d, %d): OK\n", TID, COMANDO_DEBITAR, temporario.idConta, temporario.valor);
				pthread_mutex_lock(&mutexvarcond);
				if (Ncomandos-- == 0) cond_signal(&varcondicao);
				pthread_mutex_unlock(&mutexvarcond);			
			}
		}

		else if (temporario.operacao == 2) {

			if(creditar(temporario.idConta, temporario.valor) < 0){
				printf("%s(%d, %d): Erro\n\n", COMANDO_CREDITAR, temporario.idConta, temporario.valor);
				fprintf(trab, "%d:%s(%d, %d): Erro\n", TID, COMANDO_CREDITAR, temporario.idConta, temporario.valor);
				pthread_mutex_lock(&mutexvarcond);
				if (Ncomandos-- == 0) cond_signal(&varcondicao);
				pthread_mutex_unlock(&mutexvarcond);				
			}

			else{
				printf("%s(%d, %d): OK\n\n", COMANDO_CREDITAR, temporario.idConta, temporario.valor);
				fprintf(trab, "%d:%s(%d, %d): OK\n", TID, COMANDO_CREDITAR, temporario.idConta, temporario.valor);
				pthread_mutex_lock(&mutexvarcond);
				if (Ncomandos-- == 0) cond_signal(&varcondicao);
				pthread_mutex_unlock(&mutexvarcond);
			}
		}

		
		else if (temporario.operacao == 3){
			int saldo = lerSaldo(temporario.idConta);
			if(saldo == -1){
				perror("Conta Inexistente\n\n");
				fprintf(trab, "%d:Conta Inexistente\n", TID);
				pthread_mutex_lock(&mutexvarcond);
				if (Ncomandos-- == 0) cond_signal(&varcondicao);
				pthread_mutex_unlock(&mutexvarcond);
			}
			else{
				printf("%d\n\n", saldo);
				fprintf(trab, "%d:%d\n", TID, saldo);
				pthread_mutex_lock(&mutexvarcond);
				if (Ncomandos-- == 0) cond_signal(&varcondicao);
				pthread_mutex_unlock(&mutexvarcond);
			}	
		}


		else if (temporario.operacao == 4) {

			if (transferir(temporario.idConta, temporario.idContaDestino, temporario.valor) == -1) {
				printf("Erro ao transferir valor da conta %d para a conta %d\n\n", temporario.idConta, temporario.idContaDestino);
				fprintf(trab, "%d:Erro ao transferir valor da conta %d para a conta %d\n", TID, temporario.idConta, temporario.idContaDestino);
				pthread_mutex_lock(&mutexvarcond);
				if (Ncomandos-- == 0) cond_signal(&varcondicao);
				pthread_mutex_unlock(&mutexvarcond);

			}

			else {
				printf("transferir(%d, %d, %d): OK\n\n", temporario.idConta, temporario.idContaDestino, temporario.valor);
				fprintf(trab, "%d:transferir(%d, %d, %d): OK\n", TID, temporario.idConta, temporario.idContaDestino, temporario.valor);
				pthread_mutex_lock(&mutexvarcond);
				if (Ncomandos-- == 0) cond_signal(&varcondicao);
				pthread_mutex_unlock(&mutexvarcond);
			}
		}

		pthread_mutex_unlock(&lock);
		sem_post(&sem_produtores);		//No fim de um comando ser executado o semaforo dos produtores e assinalado para informar que ha mais um
	}									//espaco disponivel no buffer para ser preenchido

	return 0;

}


int main (int argc, char** argv) {

    char *args[MAXARGS + 1];
    char buffer[BUFFER_SIZE];
    int NProcessos = 0;
    pid_t listaPids[20];
    trab = fopen("log.txt", "w");

    if(trab == NULL){
    	perror("Erro ao criar/abrir ficheiro");
    	exit(EXIT_FAILURE);
    }


    pthread_t thread_ids[NUM_TRABALHADORAS];		//Declaracao do vetor de threads que vai conter os ids das mesmas

    for(int i = 0; i< NUM_TRABALHADORAS; i++){
    	pthread_create(&thread_ids[i], NULL, executa_operacao, NULL);	//Ciclo responsavel por criar as 3 threads a executarem a funcao executa_operacao
    }

    if(sem_init(&sem_consumidores,0,0)){	//Criacao do semaforo dos consumidores, no caso de nao ser bem sucedida retorna erro e o programa encerra
    	perror("Init sem_consumidores");
    	exit(EXIT_FAILURE);
    }

    if(sem_init(&sem_produtores,0,6)){		//Criacao do semaforo dos consumidores, no caso de nao ser bem sucedida retorna erro e o programa encerra
    	perror("Init sem_produtores");
    	exit(EXIT_FAILURE);
    }

    if(pthread_cond_init(&varcondicao, NULL)) {	//A funcao pthread_cond_init devolve um numero != 0 se der erro
    	perror("Erro na criacao da variavel de condicao");
    	exit(EXIT_FAILURE);
    }

    signal(SIGUSR1, tratamentosinal);

    signal(SIGINT, tratamentosinal);

    inicializarContas();

    printf("Bem-vinda/o ao i-banco\n\n");
      
    while (1) {

        int numargs;
    
        numargs = readLineArguments(args, MAXARGS+1, buffer, BUFFER_SIZE);

        /* EOF (end of file) do stdin ou comando "sair" */

        if (numargs < 0 ||
	        (numargs > 0 && (strcmp(args[0], COMANDO_SAIR) == 0))) {
            
        	printf("O i-banco vai terminar.\n--\n");

        	if (numargs == 2 && (strcmp(args[1], "agora") == 0)) { //Comando "sair agora"

        		for(int p=0; p <= NProcessos; p++) { //mandar um signal a todos

        			int status;

        			status = kill(listaPids[p], SIGUSR1); //envia um signal a todos os filhos, e sabe os seus pid's atraves do vetor listaPids

        			if (status == -1) continue; //quer dizer que o processo ja acabou e estava a tentar mandar-lhe um signal 

        		}

    		    for (int p=0; p <= NProcessos; p++) { //percorrendo o vetor com os pid's dos filhos imprime se acabou bem ou mal

	            	int status;

	            	int pid = wait(&status); 
	            	
	            	if (pid == -1) break; //o pid retorna -1 se ja nao houver filhos, por isso sai do ciclo (break)

	            	if(WIFEXITED(status))

	            		printf("FILHO TERMINADO (PID=%d; terminou normalmente)\n", pid);
	            	else

	                    printf("FILHO TERMINADO (PID=%d; terminou abruptamente)\n", pid);

            	}

            	Var_Sair = 1;	//Coloca a variavel de saida a 1 de modo a que as threads na funcao executa_operacao saibam que tem de encerrar

            	for(int k = 0; k < NUM_TRABALHADORAS; k++){	//Ao fazermos este ciclo incrementamos o semaforo dos consumidores
            		sem_post(&sem_consumidores);			//Vai permitir passar o wait da funcao executa_operacao e terminar as threads que existam
            	}

            	for (int p = 0; p < NUM_TRABALHADORAS; p++) {

            		if (pthread_join(thread_ids[p], NULL)) {
            			perror("Ocorreu um erro a encerrar a thread.");
            		}
            		
            	}	

            	if(fclose(trab) !=0){
            		perror("Erro ao fechar ficheiro");
            	}	


           		printf("Simulacao terminada por signal\n");
           		exit(EXIT_SUCCESS);
     
            } else { //Comando "sair"

            	for (int p=0; p <= NProcessos; p++) { //no "sair" apenas espera que eles saiam e no final imprime como sairam

	            	int status;

	            	int pid = wait(&status); //isto vai retornar -1 se ja nao houver filhos
	            	if (pid == -1) break;

	            	if(!WIFEXITED(status))

	            		printf("FILHO TERMINADO (PID=%d; terminou abruptamente)\n", pid);
	            	
	                else
	                    printf("FILHO TERMINADO (PID=%d; terminou normalmente)\n", pid);
	                
	            }

            	Var_Sair = 1;			//Coloca a variavel de saida a 1 de modo a que as threads na funcao executa_operacao saibam que tem de encerrar						

            	for(int k = 0; k < NUM_TRABALHADORAS; k++){	//Ao fazermos este ciclo incrementamos o semaforo dos consumidores
            		sem_post(&sem_consumidores);			//Vai permitir passar o wait da funcao executa_operacao e terminar as threads que existam
            	}

            	for (int p = 0; p < NUM_TRABALHADORAS; p++) {

            		if (pthread_join(thread_ids[p], NULL)) {
            			perror("Ocorreu um erro a encerrar a thread.");
            		}
            		
            	}	            	

	        printf("--\nO i-banco terminou.\n");            
            
            exit(EXIT_SUCCESS);

	        }
	       }
    
    	/* Nenhum argumento; ignora e volta a pedir */
		else if (numargs == 0)
    		continue;
            
	    /* Debitar */
	    else if (strcmp(args[0], COMANDO_DEBITAR) == 0) {
	            
	            if (numargs < 3) {
	                printf("%s: Sintaxe invalida, tente de novo.\n", COMANDO_DEBITAR);
		           continue;
	            }

	            //A logica utilizada nesta parte e semelhante a logica aplicada nas restantes partes com as devidas alteracoes de valor


	            sem_wait(&sem_produtores);										//Aqui verificamos se existem posicoes livres no buffer para se colocar o comando que vai ser criado

	            comando_t comando = criaComando(1,atoi(args[1]),0, atoi(args[2]));	//Se não ficar bloqueado no wait cria o comando

	            pthread_mutex_lock(&mutexvarcond);								//O trinco da variavel de condicao protege a sua condicao partilhada
   	 	       	Ncomandos++;													//Variavel global que conta o numero de comandos que estao no buffer
   	        	pthread_mutex_unlock(&mutexvarcond);

   	        	pthread_mutex_lock(&lock);										//Fechar o trinco de modo a proteger as seguintes variaveis partilhadas

	            cmd_buffer[buff_write_idx] = comando;							//colocacao do comando criado no buffer de comandos
	            buff_write_idx = (buff_write_idx+1) % (CMD_BUFFER_DIM);

	            pthread_mutex_unlock(&lock);									//Abertura do trinco

	            sem_post(&sem_consumidores);									//Assinalar do semaforo dos consumidores a indicar que ha um comando a processar

	            
	            }

	    /* Creditar */

	    else if (strcmp(args[0], COMANDO_CREDITAR) == 0) {

	        if (numargs < 3) {
	            printf("%s: Sintaxe invalida, tente de novo.\n", COMANDO_CREDITAR);
	            continue;
	        }

	        sem_wait(&sem_produtores);

	        comando_t comando = criaComando(2,atoi(args[1]), 0, atoi(args[2]));

	        pthread_mutex_lock(&mutexvarcond);
   	        Ncomandos++;
   	        pthread_mutex_unlock(&mutexvarcond);


	        pthread_mutex_lock(&lock);

	        cmd_buffer[buff_write_idx] = comando;
	        buff_write_idx = (buff_write_idx+1) % (CMD_BUFFER_DIM);

	        pthread_mutex_unlock(&lock);

	        sem_post(&sem_consumidores);
	    }

	    /* Ler Saldo */
	    else if (strcmp(args[0], COMANDO_LER_SALDO) == 0) {

	        if (numargs < 2) {
	            printf("%s: Sintaxe invalida, tente de novo.\n", COMANDO_LER_SALDO);
	            continue;
	        }
	        sem_wait(&sem_produtores);

	        comando_t comando = criaComando(3,atoi(args[1]),0, 0);

	        pthread_mutex_lock(&mutexvarcond);
   	        Ncomandos++;
   	        pthread_mutex_unlock(&mutexvarcond);


	        pthread_mutex_lock(&lock);

	        cmd_buffer[buff_write_idx] = comando;
	        buff_write_idx = (buff_write_idx+1) % (CMD_BUFFER_DIM);

	        pthread_mutex_unlock(&lock);

	        sem_post(&sem_consumidores);
	    }

	    /* Simular */
	    else if (strcmp(args[0], COMANDO_SIMULAR) == 0) {

		pthread_mutex_lock(&mutexvarcond);

		while (Ncomandos != 0) {								//quando o buffer estiver vazio ha apenas simulares para executar

			pthread_cond_wait(&varcondicao, &mutexvarcond);
		}

   	    pthread_mutex_unlock(&mutexvarcond);

	    pid_t pid=fork();

	    listaPids[NProcessos++] = pid;
       
	    int numAnos;

	    if(numargs != 2){
	    	printf("%s: Sintaxe invalida, tente de novo\n", COMANDO_SIMULAR);
	    	continue;
	    }

	    if(pid==0){    
	    	numAnos = atoi(args[1]);
	    	simular(numAnos);
	    	exit(EXIT_SUCCESS);
	    }

	}
	    
	    /*Transferir*/

	    else if (strcmp(args[0], COMANDO_TRANSFERIR) == 0) {

	    	if(numargs != 4){
	    		printf("%s: Sintaxe invalida, tente de novo\n\n", COMANDO_TRANSFERIR);
	    		continue;
	    	}

	    	sem_wait(&sem_produtores);

			comando_t comando = criaComando(4, atoi(args[1]), atoi(args[2]), atoi(args[3]));

			pthread_mutex_lock(&lock);
			cmd_buffer[buff_write_idx] = comando;
			buff_write_idx = (buff_write_idx+1) % (CMD_BUFFER_DIM);
			Ncomandos++;
			pthread_mutex_unlock(&lock);
			
			sem_post(&sem_consumidores);

	    }

	    else {
	      printf("Comando desconhecido. Tente de novo.\n");
	    }

	  } 
	}


