Ao iniciar os testes, percebi um erro em buffer.c, onde os produtores e consumidores não liberavam o semáforo de exclusão caso existam produtores ou consumidores para acordar. 
Trocar

else {
    checkState("ProducerEnds");
    sem_post(&exc);
  }

por

checkState("ProducerEnds");
sem_post(&exc);

Um exemplo de teste que aponta o erro:
ProducerInitialized
ConsumerInitialized
ConsumerStarts
ConsumerWaiting
ProducerStarted
ProducerWriteStarts
ProducerWriteEnds
ProducerFreedConsumer

DeadLock detected!!
Expected event 9 called ProducerEnds

Com a correção:
ProducerInitialized
ConsumerInitialized
ConsumerStarts
ConsumerWaiting
ProducerStarted
ProducerWriteStarts
ProducerWriteEnds
ProducerFreedConsumer
ProducerEnds

e o teste passa.

Outro erro percebido foi o mal uso do semáforo de exclusão mútua nas áreas críticas dos produtores/consumidores em suas respectivas escritas e leituras. O erro:

sem_post(&exc);

buf[nextSlot] = item;
faltaLer[nextSlot] = NUMCONS;
nextSlot = (nextSlot + 1) % BUFSIZE;
freeSlots--;
totalEscritos++;

sem_wait(&exc);

a correção:

sem_wait(&exc);

buf[nextSlot] = item;
faltaLer[nextSlot] = NUMCONS;
nextSlot = (nextSlot + 1) % BUFSIZE;
freeSlots--;
totalEscritos++;

Casos de teste:
Produtor escreve e não libera consumidor ou produtor (./readerWriter 1 1 1 1)

ProducerInitialized *
ProducerStarted *
ProducerWriteStarts *
ProducerWriteEnds *
ProducerEnds *

Produtor escreve e libera consumidor (./readerWriter 1 1 1 1)

ProducerInitialized *
ProducerStarted *
ConsumerInitialized *
ConsumerStarts *
ConsumerWaiting *
ProducerWriteStarts *
ProducerWriteEnds *
ProducerFreedConsumer *
ProducerEnds *

Produtor escreve e libera produtor
!!! Este teste não é necessário, um produtor nunca vai liberar um produtor pois nunca irá escrever algo e liberar espaço para outro produtor. Aproveitei e removi esta parte do código na função deposita

ProducerInitialized *
ProducerStarted *
ProducerWriteStarts *
ProducerInitialized 1
ProducerStarted 1
ProducerWaiting 1
ProducerWriteEnds *
ProducerFreedProducer *
ProducerEnds *

Consumidor libera produtor (./readerWriter 1 2 1 1)
ProducerInitialized *
ProducerStarted *
ProducerWriteStarts *
ProducerInitialized 1
ProducerStarted 1
ProducerWaiting 1
ProducerWriteEnds *
ProducerEnds *
ConsumerInitialized *
ConsumerStarts *
ConsumerReadingStarts *
ConsumerReadingEnds *
ConsumerFreedProducer *
ConsumerEnds *

Consumidor tenta ler o mesmo item duas vezes (./readerWriter 1 1 2 2)
ProducerInitialized *
ProducerStarted *
ProducerWriteStarts *
ProducerWriteEnds *
ProducerEnds *
ConsumerInitialized 1
ConsumerStarts 1
ConsumerReadingStarts 1
ConsumerReadingEnds 1
ConsumerEnds 1
ConsumerInitialized 1
ConsumerStarts 1
ConsumerWaiting 1

Dois consumidores leem o mesmo item (./readerWriter 1 1 2 2)
ProducerInitialized *
ProducerStarted *
ProducerWriteStarts *
ProducerWriteEnds *
ProducerEnds *
ProducerInitialized 1
ProducerStarted 1
ProducerWaiting 1
ConsumerInitialized 1
ConsumerStarts 1
ConsumerReadingStarts 1
ConsumerReadingEnds 1
ConsumerEnds 1
ConsumerInitialized !1
ConsumerStarts !1
ConsumerReadingStarts !1
ConsumerReadingEnds !1
ConsumerFreedProducer !1
ConsumerEnds !1
ProducerWriteStarts 1
ProducerWriteEnds 1
ProducerEnds 1


Dois consumidores leem o mesmo item e liberam produtor (./readerWriter 1 1 2 2)
ProducerInitialized *
ProducerStarted *
ProducerWriteStarts *
ProducerWriteEnds *
ProducerEnds *
ProducerInitialized *
ProducerStarted *
ProducerWaiting *
ConsumerInitialized 1
ConsumerStarts 1
ConsumerReadingStarts 1
ConsumerReadingEnds 1
ConsumerEnds 1
ConsumerInitialized !1
ConsumerStarts !1
ConsumerReadingStarts !1
ConsumerReadingEnds !1
ConsumerFreedProducer !1
ConsumerEnds !1