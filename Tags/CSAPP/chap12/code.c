int readcnt;
/* Initially 0 */
sem_t mutex, w; /* Both initially 1 */
void reader(void) {
  while (1) {
    P(&mutex);
    readcnt++;
    if (readcnt == 1) /* First in */
      P(&w);
    V(&mutex);

    /* Reading happens here */

    P(&mutex);
    readcnt--;
    if (readcnt == 0) /* Last out */
      V(&w);
    V(&mutex);
  }
}

Writers : void writer(void) {
  while (1) {
    P(&w);
    /* Writing here */
    V(&w);
  }
}
