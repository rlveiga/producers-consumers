# Produtores e Consumidores

Este repositório contém código relacionado ao Trabalho 1 de INF1406 - Programação Distribuída e Concorrente.

Este projeto faz uso do módulo de testes StateManager, cuja documentação pode ser acessada clicando [aqui](www.google.com).

## Execução

Para compilar o programa, abrir a pasta **/src** e rodar (para usuários Unix)

`gcc -o main main.c buffer.c ../StateManager/stateManager.c -pthread -I/$LUA_CDIR -llua5.3`

Obs: [O usuário deve instalar Lua 5.3 na máquina, caso já não esteja instalado.](https://github.com/AnnaLeticiaAlegria/Testes_Concorrentes#como-utilizar-o-m%C3%B3dulo-statemanager)

E para executar: 

`./main <tamanho-buffer> <numero-produtores> <numero-consumidores> <numero-itens>`

Obs: O código referente aos testes está comentado, para executar os testes basta remover o comentário na chamada `initializeManager` em `main.c` e nas chamadas para `checkState` em `buffer.c`

## Atualizações recentes

Ao iniciar um dos testes, percebi um erro em buffer.c, onde os produtores e consumidores não liberavam o semáforo de exclusão caso existam produtores ou consumidores para "acordar".

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

Após trocar

```
else {
    checkState("ProducerEnds");
    sem_post(&exc);
  }
```

por

```
checkState("ProducerEnds");
sem_post(&exc);
```

O teste passa:

ProducerInitialized
ConsumerInitialized
ConsumerStarts
ConsumerWaiting
ProducerStarted
ProducerWriteStarts
ProducerWriteEnds
ProducerFreedConsumer
ProducerEnds

Outro erro percebido foi o mal uso do semáforo de exclusão mútua nas áreas críticas dos produtores/consumidores em suas respectivas escritas e leituras. O erro:

```
sem_post(&exc);

buf[nextSlot] = item;
faltaLer[nextSlot] = NUMCONS;
nextSlot = (nextSlot + 1) % BUFSIZE;
freeSlots--;
totalEscritos++;

sem_wait(&exc);
```

foi corrigido para:

```
sem_wait(&exc);

buf[nextSlot] = item;
faltaLer[nextSlot] = NUMCONS;
nextSlot = (nextSlot + 1) % BUFSIZE;
freeSlots--;
totalEscritos++;
```

## Casos de teste

Os casos de teste estão numerados para fácil identificação na pasta de testes. Para trocar o teste a ser executado, alterar o primeiro parâmetro da chamada para a função `initializeManager` em `main.c`.

1. Produtor escreve e não libera consumidor ou produtor (./readerWriter 1 1 1 1)

```
ProducerInitialized *
ProducerStarted *
ProducerWriteStarts *
ProducerWriteEnds *
ProducerEnds *
```

2. Produtor escreve e libera consumidor (./readerWriter 1 1 1 1)

```
ProducerInitialized *
ProducerStarted *
ConsumerInitialized *
ConsumerStarts *
ConsumerWaiting *
ProducerWriteStarts *
ProducerWriteEnds *
ProducerFreedConsumer *
ProducerEnds *
```

3. Produtor escreve e libera produtor
!!! Este teste falha e não é necessário, um produtor nunca vai liberar um produtor pois nunca irá escrever algo e liberar espaço para outro produtor. Aproveitei e removi esta parte do código na função `deposita`.

```
ProducerInitialized *
ProducerStarted *
ProducerWriteStarts *
ProducerInitialized 1
ProducerStarted 1
ProducerWaiting 1
ProducerWriteEnds *
ProducerFreedProducer *
ProducerEnds *
```

4. Consumidor libera produtor (./readerWriter 1 2 1 1)

```
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
```

5. Consumidor tenta ler o mesmo item duas vezes (./readerWriter 1 1 2 2)

```
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
```

6. Dois consumidores leem o mesmo item (./readerWriter 1 1 2 2)

```
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
ConsumerInitialized !1
ConsumerStarts !1
ConsumerReadingStarts !1
ConsumerReadingEnds !1
ConsumerEnds !1
```


7. Dois consumidores leem o mesmo item e liberam produtor (./readerWriter 1 1 2 2)

```
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
```
