int buffer[TAMBUF]; // inicializa com nulo

int currentSlot = 0;
int nextSlot = 0;
int faltaler[TAMBUF]; // inicializa tudo com 0

int lidos[NUMCONS]; // inicializa com 0
int escritos = 0;
int totallidos = 0;

void iniciabuffer (int numpos, int numprod, int numcons) {
  inicia buffer...
}

void deposita(int item) {
  // must have an empty slot
  <
  await 
  emptySlots > 0;
    buf[nextSlot] = item;
    faltaler[nextSlot] = NUMCONS;
    nextSlot = (currentSlot + 1) % TAMBUF;
    escritos++;
  >
}

void consome(int meuid) {
  // must have a readable item
  // must not have read the item already
  <
  await 
  faltaler[currentSlot] > 0 and
  lidos[meuid] < escritos;
    item = buf[currentSlot];
    faltaler[currentSlot] = faltaler[currentSlot] - 1;
    totallidos++;
    lidos[meuid] = lidos[meuid] + 1;
    >
}

void finalizabuffer (int numpos, int numprod, int numcons) {
  // all products must have been read by all consumers
  <await totallidos == numcons * NPRODUCTS * NUMPROD>
  finaliza buffer...
}