/*
Module: stateManager.c
Author: Anna Leticia Alegria
Last Modified at: 17/06/2021

----------------------------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------------------------
Description: This module contains the State Manager functions. 
The Manager works reading the passed file that contains the desired order of the events. It calls the module 
readStates.lua to read this file and return an array of the desired events (statesArray) and an array of the states id
(statesIdArray), in which each entry corresponds to the state on the same position in statesArray. This is done in
the initalizeManager function.

The documentation explains how the state's file should be done.

After the statesArray and the statesIdArray are ready, the user can call the function checkState, passing the desired
state/event. This state must exist in the state's file. The program checks if the desired event is the next on the
statesArray and if it's id corresponds to the next state's id.
This way, the program can control the event's order and make sure that the threads follow this order.

At the end of the user's program, the finalizeManager function should be called to free all the memory allocated
by this module.
----------------------------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------------------------
*/

#include "stateManager.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <signal.h>
#include <unistd.h>

/* 
----------------------------------------------------------------------------------------------------------------------
Global variable's declaration
----------------------------------------------------------------------------------------------------------------------
*/

pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
pthread_mutex_t conditionLock = PTHREAD_MUTEX_INITIALIZER;

int currentState = 0; // Position of the current state in the statesArray and in the statesIdArray
int totalStates = 0;  // Total state's sequence length

const char ** statesArray; // Array with the names of each event of the sequence. It's length is equal to totalStates 
int * statesIdArray;       // Array with the ids of each event of the sequence. It's length is equal to totalStates 
pthread_t * threadIdArray; // Array with the thread's id of the user's program. It's length is equal to the number of threads


/*
----------------------------------------------------------------------------------------------------------------------
Encapsulated function's declaration
----------------------------------------------------------------------------------------------------------------------
*/

void getLuaResults (lua_State *LState);
int compareStates (const char * state, int currentState);
void signalHandler(int signum);

/*
----------------------------------------------------------------------------------------------------------------------
Function: initializeManager
Parameters: 
  -> fileName: the name of the file with the states' order
  -> nThreads: user's program's total number of threads
Returns: nothing

Description: This function loads the Lua file called readStates.lua, located at readStatesFilePath. It exits in case
this file doesn't exist. Next, the function prepares the stack to call the Lua functions in readStates.lua.
Since the function readStates requires a string with the path of the file, the program puts this variable in the stack,
after putting the function's name. After that, it call lua_pcall to call the Lua function.
Then, call this module's function getLuaResults. This function obtains the returns of the Lua function and places at
the global variables of this module.
After closing the LuaState, this function allocates the threadIdArray with length of nThreads and inserts 0 at each
position.
Lastly, it sets an alarm with time 'deadLockDetectTime' (which is, by default, 5 seconds)
----------------------------------------------------------------------------------------------------------------------
*/
void initializeManager (char * fileName, int nThreads) {
  lua_State *LState;

  /* Set SIGALRM handler */
  signal(SIGALRM,signalHandler);

  /* Open Lua State */
  LState = luaL_newstate();
  luaL_openlibs(LState); 

  /* Load Lua file */
  if (luaL_loadfile(LState, readStatesFilePath)) {
    printf("Error opening lua file\n");
    exit(0);
  }

  /* Priming run */
  int pcall_result = lua_pcall(LState, 0, 0, 0);
  if (pcall_result) {
    const char* errmsg = lua_tostring(LState, -1);
    printf("Error in priming run %d %s\n", pcall_result, errmsg);
    exit(0);
  }

  /* Put the function's name and its parameters in the stack */
  lua_getglobal(LState, "readStates");
  lua_pushlstring(LState, fileName, strlen(fileName));  

  /* Call the function on the stack, giving 1 parameter and expecting 3 values to be returned */
  if (lua_pcall(LState, 1, 3, 0)) {
    printf("Error during readStates call\n");
    exit(0);
  }

  /* Pop the results from calling Lua function */
  getLuaResults (LState);

  /* Close Lua State */
  lua_close(LState);

  /* Allocate threadIdArray */
  threadIdArray = (pthread_t*) malloc (nThreads * sizeof(pthread_t));
  if (threadIdArray == NULL) {
    printf("Error in threadIdArray malloc\n");
    exit(0);
  }

  /* Insert 0 at each position of threadIdArray */
  for(int i = 0; i<nThreads; i++){
    threadIdArray[i] = 0;
  }

  /* Set alarm of 'deadLockDetectTime' seconds */
  alarm(deadLockDetectTime);
}


/*
----------------------------------------------------------------------------------------------------------------------
Function: checkState
Parameters: 
  -> state: name of the state that wants to go next
Returns: nothing

Description: This functions checks if the given state is the next state in the statesArray order and if it fullfills 
the id's condition. For more details about the id's condition, check the function compareStates.
If so, it prints the state, updates the current state and calls the pthread function that sends a message in broadcast
to all the waiting threads so they can stop waiting and check if its their turn now.
If it isn't this state's turn, the thread keeps waiting until another thread sinalize that it can stop waiting.

There is an alarm with time of 'deadLockDetectTime' seconds declared everytime a state begins it's turn. If this time 
runs out, it means no other state started a turn for the last 'deadLockDetectTime' seconds. This means that every 
thread is waiting for the turn of their state and none of them has been accepted. This indicate that the sequence
in the state File is not a valid sequence for the user's program. So, the handler of the SIGALRM ends the manager
and the user's program.
----------------------------------------------------------------------------------------------------------------------
*/
void checkState (const char * state) {

  while (1) {
    pthread_mutex_lock(&conditionLock); // P(conditionLock)

    /* In case there are no states left */
    if (currentState == totalStates) {
      pthread_mutex_unlock(&conditionLock); // V(conditionLock)
      pthread_exit(NULL); // End thread
    }

    /* Check if it's this state's turn */
    if(compareStates(state, currentState)) {
      printf("%s\n", state);
      currentState ++;
      pthread_cond_broadcast(&condition); // Tell to the awaiting threads that they can go on
      pthread_mutex_unlock(&conditionLock); // V(conditionLock)
      alarm(deadLockDetectTime);
      return;
    }

    pthread_cond_wait(&condition, &conditionLock); // Thread keeps waiting until another thread tells to go on

    pthread_mutex_unlock(&conditionLock); // V(conditionLock)
  }
  
}


/*
----------------------------------------------------------------------------------------------------------------------
Function: finalizeManager
Parameters: none
Returns: nothing

Description: This function releases all the variables allocated by the function initializeManager. It must be called
at the end of the user's program.
----------------------------------------------------------------------------------------------------------------------
*/
void finalizeManager (void) {
  free(threadIdArray);
  free(statesArray);
  free(statesIdArray);
  alarm(0);
}

/*
----------------------------------------------------------------------------------------------------------------------
Function: getLuaResults
Parameters: 
  -> LState: Lua State opened by initalizeManager
Returns: nothing

Description: This function is called after the Lua function is called. It gets every return of the Lua function that
is on the stack. The Lua function readStates returns the value in the following order: stateIdArray, stateNameArray, 
length of stateNameArray. So, after the function is called, the stack looks like this:

  ----------------------------
  | lenght of stateNameArray |
  ----------------------------
  |      stateNameArray      |
  ----------------------------
  |       stateIdArray       |
  ----------------------------

It means that the results must be popped out of the stack in the following order: length of stateNameArray, 
stateNameArray, stateIdArray.
Getting the length of the stateNameArray, the statesNameArray and the statesIdArray can be allocated with this length.
----------------------------------------------------------------------------------------------------------------------
*/
void getLuaResults (lua_State *LState) {
  int i = 0;

  /* In readStates.lua, the array size is the last value returned, so it is on the top of the stack */
  totalStates = lua_tonumber(LState, -1);
  lua_pop(LState,1);

  /* Allocate statesArray */
  statesArray = (const char **) malloc ((totalStates) * sizeof(const char*));
  if (statesArray == NULL) {
    printf("Error in statesArray malloc\n");
    exit(0);
  }

  /* Allocate statesIdArray */
  statesIdArray = (int *) malloc ((totalStates) * sizeof(int));
  if (statesIdArray == NULL) {
    printf("Error in statesIdArray malloc\n");
    exit(0);
  }

  lua_pushnil(LState); 

  /* Since statesArray is an array, the function has to pop everything at this address until it has nothing (which means
  that all the array has been popped) */
  while (lua_next(LState, -2) != 0) {
    /* Since is a Lua table, it has key and value. But in this case, the key is a sequential number in range of 
    (1, #statesArray). This function uses 'key' (at index -2) and 'value' (at index -1). The function needs only the
    value, therefore, it only needs what is at index -1 */
    statesArray[i] = lua_tostring(LState, -1);
    lua_pop(LState, 1);
    i++;
  }

  lua_pushnil(LState);
  i = 0;

  /* Since statesIdArray is an array, the function has to pop everything at this address until it has nothing 
  (which means that all the array has been popped) */
  while (lua_next(LState, -3) != 0) {
    /* Same case as above */
    statesIdArray[i] = lua_tonumber(LState, -1);
    lua_pop(LState, 1);
    i++;
  }
}

/*
----------------------------------------------------------------------------------------------------------------------
Function: compareStates
Parameters: 
  -> state: name of the state that wants to go next
  -> currentState: position of the currentState in statesArray
Returns:
  -> 1: if the given state is the next state
  -> 0: if the given state is not the next state. Therefore, it needs to wait its turn

Description: This function first compares the given state name with the current state's name to see if it can be the 
next state to go on. If not, returns 0. If so, continue to check the states id.

The state id is defined by the user on the state's file order. If this state can be performed by any thread, it's id
is equal to 0. If it must be performed by a specific thread, this id contains an integer greater than 0.(*Note 1) 
If it can be performed by any thread but cannot be performed by a specific thread, this id contains an integer lower 
than 0.(*Note 2)

If the thread is the first one passing the name and the positive id's condition, it assignes it's id to the 
threadIdArray at the (id - 1) position.
If the thread passes the name's and the id's condition, the function returns 1.

*Note 1: The user must assign a positive number to this state that is lower or equal to the number of threads. This
number (called id) does not guarantee that this thread is the (id)th thread to be executed, it means that this thread
will be considered the (id)th when it reachs this function. This id is useful when the user wants to guarantee that
differents states/events will be executed by the same thread.

*Note 2: To use this resource, the user must be assigned a thread with the specified id. (For example, if it assigns -1
to a state, it must be any state before assigned to the id 1). Is important to notice that any thread but the specified
can execute this state. Works on changing that are being done.
----------------------------------------------------------------------------------------------------------------------
*/
int compareStates (const char * state, int currentState) {

  /* First, check the state's name */
  if (!strcmp(state, statesArray[currentState])) {

    /* Second, check if it cannot be any threadId */
    if (statesIdArray[currentState] != 0) {
      pthread_t currentThreadId = pthread_self(); // Get the thread id
      pthread_t targetThreadId = threadIdArray[abs(statesIdArray[currentState]) - 1]; // Get the currentState's thread id

      /* Then, check the state's id */
      if (statesIdArray[currentState] > 0) { // it means that it must be the state with this id

        /* If there was no thread assigned to this id yet, it assignes itself*/
        if (targetThreadId == 0) {
          threadIdArray[abs(statesIdArray[currentState]) - 1] = currentThreadId;
          return 1;
        }
        return pthread_equal(currentThreadId, targetThreadId);
      }

      /* If reached this point, it means that it cannot be the state with this id (statesIdArray[currentState] < 0) */
      if (!pthread_equal(currentThreadId, targetThreadId)) {
        return 1;
      }
    }
    else {
      /* If it can be any id, it returns 1 since it passed already by the name's condition */
      return 1;
    }
  }

  /* In case it didn't passed any of the conditions, returns 0 */
  return 0;
}

/*
----------------------------------------------------------------------------------------------------------------------
Function: signalHandler
Parameters: 
  -> signum: number of the signal that caused this handler to be called
Returns: nothing

Description: This handler is called whenever the SIGALRM reaches the time that it was specified. If another call of
alarm is done, the time will reset (only one alarm can be active at the same time)
----------------------------------------------------------------------------------------------------------------------
*/
void signalHandler(int signum){
  printf("\n\n\nDeadLock detected!!\n");
  printf("Expected event %d called %s\n\n\n", currentState + 1, statesArray[currentState]);
  finalizeManager ();
  exit(0);
}