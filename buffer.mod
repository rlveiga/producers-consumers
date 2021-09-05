int nw[TAMBUF];
int nr[TAMBUF];

int aserlido[NPRODUCTS]; // indicates buffer slot in which each produced item is located, initializes with -1

int lidos[NUMCONS];
int faltaler[TAMBUF];
int escritos = 0;
int totallidos = 0;

sem e[TAMBUF];

void iniciabuffer (int numpos, int numprod, int numcons) {
  inicia buffer...
}

void deposita(int item) {
  // must be the only producer or reader trying to access buffer slot
  // buffer slot must be empty
  <await nw[item] == 0 and nr[item] == 0 and faltaler[item] == 0; nw[item] = nw[item] + 1>
  deposita item...
  P(e[item]);
  faltaler[item] = NUMCONS;
  aserlido[escritos] = item;
  V(e[item]);
  <nw[item] = nw[item] - 1>
}

void consome(int meuid) {
  // must have a readable item
  <await aserlido[lidos[meuid]] > -1>
  // must be the only reader trying to access buffer slot
  <await nr[aserlido] == 0; nr[aserlido] = nr[aserlido] = nr[aserlido] + 1>
  consome item...
  P(e[aserlido]);
  faltaler[aserlido] = faltaler[aserlido] - 1;
  totallidos = totallidos + 1;
  V(e[aserlido]);
  <nr[aserlido] = nr[aserlido] - 1>
}

void finalizabuffer (int numpos, int numprod, int numcons) {
  // all products must have been read by all consumers
  <await totallidos == numcons * NPRODUCTS>
  finaliza buffer...
}