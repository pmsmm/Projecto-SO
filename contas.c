#include "contas.h"
#include <unistd.h>                   
#include <stdio.h>                    
#include <stdlib.h>
#include <pthread.h>

#define atrasar() sleep(ATRASO)
#define COMANDO_DEBITAR "debitar"
#define COMANDO_CREDITAR "creditar"
#define COMANDO_LER_SALDO "lerSaldo"
#define COMANDO_SIMULAR "simular"
#define COMANDO_SAIR "sair"


pthread_mutex_t cadeados[NUM_CONTAS];		     
int contasSaldos[NUM_CONTAS];
int sinal = 0;
int somaCreditos = 0;
FILE *sim;

int contaExiste(int idConta) {
  return (idConta > 0 && idConta <= NUM_CONTAS);
}

void inicializarContas() {
  int i;
  for (i=0; i<NUM_CONTAS; i++)
    contasSaldos[i] = 0;
}

int debitar(int idConta, int valor) {
  atrasar();
  if (!contaExiste(idConta)) {
    return -1;
  }
  pthread_mutex_lock(&cadeados[idConta-1]);
  if (contasSaldos[idConta - 1] < valor) {
    pthread_mutex_unlock(&cadeados[idConta-1]);
    return -1;
  }

  atrasar();
  contasSaldos[idConta - 1] -= valor;
  pthread_mutex_unlock(&cadeados[idConta-1]);
  return 0;
}

int creditar(int idConta, int valor) {
  atrasar();
  if (!contaExiste(idConta)){
    return -1;
  }

  pthread_mutex_lock(&cadeados[idConta-1]);
  contasSaldos[idConta - 1] += valor;
  somaCreditos += valor;
  pthread_mutex_unlock(&cadeados[idConta-1]);
  return 0;
}

int lerSaldo(int idConta) {             
  atrasar();
  if (!contaExiste(idConta)){
    return -1;
  }
  pthread_mutex_lock(&cadeados[idConta-1]);
  int saldo = contasSaldos[idConta - 1];
  pthread_mutex_unlock(&cadeados[idConta-1]);
  return saldo;
}


int transferir(int idorigem, int iddestino, int valor) {
  
  if (!contaExiste(idorigem) || (!contaExiste(iddestino))) return -1;

  if (contasSaldos[idorigem-1] < valor) return -1;

  pthread_mutex_lock(&cadeados[idorigem-1]);
  pthread_mutex_lock(&cadeados[iddestino-1]);

  contasSaldos[idorigem-1]-= valor;
  contasSaldos[iddestino-1] += valor;

  pthread_mutex_unlock(&cadeados[iddestino-1]);
  pthread_mutex_unlock(&cadeados[idorigem-1]);

  return 0;

}




int simular(int numAnos) {
    char filename[50];
    sprintf(filename, "i-banco-sim-%d.txt", getpid());
    sim = fopen(filename, "w");

    if(numAnos<0) {

        fprintf(sim, "Numero de anos tem de ser maior ou igual a 0\n");
        return -1;
    }

    else {

        for(int i=0; i<=numAnos && sinal == 0; i++) {

            fprintf(sim, "SIMULACAO: Ano %d\n", i);
            fprintf(sim, "=================\n");

            for(int k=1; k <= NUM_CONTAS; k++){

                if(i==0)

                    fprintf(sim, "Conta %d, Saldo %d\n", k, lerSaldo(k));

                else {

                    creditar(k, lerSaldo(k)*(0.1));
                    debitar(k, 1);
                    fprintf(sim, "Conta %d, Saldo %d\n", k, lerSaldo(k));
                }
            }
        }
    }

    if(fclose(sim) != 0){
      perror("Erro ao fechar o ficheiro");
    }

    return 1;
    
}




void tratamentosinal(int s) {

  sinal = 1;
}

