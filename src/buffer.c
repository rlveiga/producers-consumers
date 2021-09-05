#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

#include "../StateManager/stateManager.h" // Require StateManager for testing

int* buf;
int BUFSIZE, NUMCONS, NUMPROD;

int nextSlot = 0;

int freeSlots;

int totalEscritos = 0; int totalLidos = 0;
int* faltaLer;
int* lidos;

sem_t w;
int dw;

sem_t r;
int dr;

sem_t exc;

void iniciabuffer(int numpos, int numprod, int numcons) {
  buf = (int*)malloc(sizeof(int) * numpos);
  faltaLer = (int*)malloc(sizeof(int) * numpos);
  lidos = (int*)malloc(sizeof(int) * numcons);

  BUFSIZE = numpos;
  NUMCONS = numcons;
  NUMPROD = numprod;
  freeSlots = BUFSIZE;

  for(int i = 0; i < numpos; i++) {
    faltaLer[i] = 0;
  }

  for(int i = 0; i < numcons; i++) {
    lidos[i] = 0;
  }

  sem_init(&w, 1, 0);
  sem_init(&r, 1, 0);
  sem_init(&exc, 1, 1);
}

void finalizabuffer(int numpos, int numprod, int numcons) {
  free(buf);
  free(faltaLer);
  free(lidos);
}

void deposita (int item) {
  checkState("ProducerInitialized");

  sem_wait(&exc);

  checkState("ProducerStarted");

  if(freeSlots == 0) {
    dw++;
    sem_post(&exc);
    sem_wait(&w);
    checkState("ProducerWaiting");
  }

  checkState("ProducerWriteStarts");

  sem_wait(&exc);

  buf[nextSlot] = item;
  faltaLer[nextSlot] = NUMCONS;
  nextSlot = (nextSlot + 1) % BUFSIZE;
  freeSlots--;
  totalEscritos++;

  checkState("ProducerWriteEnds");
  
  // Check for delayed readers
  if(freeSlots < BUFSIZE && dr > 0) {
    dr--;
    sem_post(&r);
    checkState("ProducerFreedConsumer");
  }

  checkState("ProducerEnds");
  sem_post(&exc);
}

int consome (int meuid) {
  checkState("ConsumerInitialized");

  sem_wait(&exc);

  checkState("ConsumerStarts");

  if(lidos[meuid] == totalEscritos || freeSlots == BUFSIZE) {
    dr++;
    sem_post(&exc);
    checkState("ConsumerWaiting");
    sem_wait(&r);
  }

  checkState("ConsumerReadingStarts");

  sem_wait(&exc);

  int currentSlot = lidos[meuid] % BUFSIZE;
  int item = buf[currentSlot];
  faltaLer[currentSlot] = faltaLer[currentSlot] - 1;
  totalLidos++;
  lidos[meuid] = lidos[meuid] + 1;

  if(faltaLer[currentSlot] == 0) {
    freeSlots++;
  }

  checkState("ConsumerReadingEnds");

  // Check for delayed writers
  if(freeSlots > 0 && dw > 0) {
    dw--;
    sem_post(&w);
    checkState("ConsumerFreedProducer");
  }

  // Check for delayed readers
  else if(freeSlots < BUFSIZE && dr > 0) {
    dr--;
    sem_post(&r);
    checkState("ConsumerFreedConsumer");
  }

  sem_post(&exc);
  checkState("ConsumerEnds");

  return item;
}
