#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>
#include <opt-A1.h>

/* 
 * This simple default synchronization mechanism allows only creature at a time to
 * eat.   The globalCatMouseSem is used as a a lock.   We use a semaphore
 * rather than a lock so that this code will work even before locks are implemented.
 */

/* 
 * Replace this default synchronization mechanism with your own (better) mechanism
 * needed for your solution.   Your mechanism may use any of the available synchronzation
 * primitives, e.g., semaphores, locks, condition variables.   You are also free to 
 * declare other global variables if your solution requires them.
 */

/*
 * replace this with declarations of any synchronization and other variables you need here
 */
static struct semaphore *globalCatMouseSem;

#define CAT 0
#define MOUSE 1
#define NONE 2
#define SPECIES

struct semaphore *bowls_sem;

struct cv *cats_eating_cv;
struct cv *mice_eating_cv;
struct cv *bowl_occupied_cv;

struct lock *bowls_lock;
struct lock *all_bowls_lock;

volatile int species_counter[2];
volatile int **bowl_waiting;

/* 
 * The CatMouse simulation will call this function once before any cat or
 * mouse tries to each.
 *
 * You can use it to initialize synchronization and other variables.
 * 
 * parameters: the number of bowls
 */
void
catmouse_sync_init(int bowls)
{
  /* replace this default implementation with your own implementation of catmouse_sync_init */

  (void)bowls; /* keep the compiler from complaining about unused parameters */
  globalCatMouseSem = sem_create("globalCatMouseSem",1);

  if (globalCatMouseSem == NULL) {
    panic("could not create global CatMouse synchronization semaphore");
  }

  #if opt-A1
    // dynamically bowls_lock allocate, bowl_occupied_cv array
    bowls_lock = malloc(bowls * sizeof(int *));
    bowl_occupied_cv = malloc(bowls * sizeof(int *));

    bowls_sem = sem_create("bowls_sem", bowls);
    all_bowls_lock = lock_create("all_bowls_lock");
    cats_eating_cv = cv_create("cats_eating_cv");
    mice_eating_cv = cv_create("mice_eating_cv");

    int i;
    
    for (i = 0; i < bowls; i++){
      bowls_lock[i] = lock_create((char)i);
      bowl_occupied_cv[i] = cv_create((char)i);
    }

    SPECIES = NONE;

    // dynamically allocate 2-dimensional bowl_waiting array
    bowl_waiting = malloc(bowls * sizeof(int *)); 
  #endif

  return;
}

/* 
 * The CatMouse simulation will call this function once after all cat
 * and mouse simulations are finished.
 *
 * You can use it to clean up any synchronization and other variables.
 *
 * parameters: the number of bowls
 */
void
catmouse_sync_cleanup(int bowls)
{
  /* replace this default implementation with your own implementation of catmouse_sync_cleanup */
  (void)bowls; /* keep the compiler from complaining about unused parameters */
  KASSERT(globalCatMouseSem != NULL);
  sem_destroy(globalCatMouseSem);

  #if opt-A1
    KASSERT(bowls_sem != NULL);
    KASSERT(all_bowls_lock != NULL);
    KASSERT(cats_eating_cv != NULL);
    KASSERT(mice_eating_cv != NULL);

    sem_destroy(bowls_sem);
    lock_destroy(all_bowls_lock);
    cv_destroy(cats_eating_cv);
    cv_destroy(mice_eating_cv);

    int i;
    
    for (i = 0; i < bowls; i++){
      bowls_lock[i] = lock_destroy((char)i);
      bowl_occupied_cv[i] = cv_destroy((char)i);
    }
    
  #endif
}


/*
 * The CatMouse simulation will call this function each time a cat wants
 * to eat, before it eats.
 * This function should cause the calling thread (a cat simulation thread)
 * to block until it is OK for a cat to eat at the specified bowl.
 *
 * parameter: the number of the bowl at which the cat is trying to eat
 *             legal bowl numbers are 1..NumBowls
 *
 * return value: none
 */

void
cat_before_eating(unsigned int bowl) 
{
  /* replace this default implementation with your own implementation of cat_before_eating */
  (void)bowl;  /* keep the compiler from complaining about an unused parameter */
  KASSERT(globalCatMouseSem != NULL);
  P(globalCatMouseSem);

  #if opt-A1
    // down semaphore on bowls available
    KASSERT(bowls_sem != NULL);
    P(bowls_sem);

    lock_acquire(all_bowls_lock);

    // no one is currently eating
    if (SPECIES == NONE) {
      SPECIES = CAT;
      KASSERT(species_counter[CAT] == 0);
    }

    // cats currenly eating
    else if (SPECIES == CAT) {
      // wait while other cats are eating desired bowl
      while(SPECIES == CAT && bowl_waiting[CAT][bowl] > 0) {
        cv_wait(&bowl_occupied_cv[bowl], &bowls_lock[bowl]);
        // cv_wait(&cats_eating_cv, &bowls_lock[bowl]);
      }

    }

    // mice currently eating
    else {
      while(SPECIES == MOUSE) {
        // cv_wait(&bowl_occupied_cv[bowl], &bowls_lock[bowl]);
        cv_wait(&mice_eating_cv, &bowls_lock[bowl]);
      }
      // mice done eating
      if (SPECIES != MOUSE) {
        lock_acquire(all_bowls_lock);
        // SPECIES = CAT;
        SPECIES = NONE;
        KASSERT(species_counter[CAT] == 0);
      }
    }

    // increment number of cats waiting for bowl
    bowl_waiting[CAT][bowl]++;
    species_counter[CAT]++;

    lock_release(bowls_lock);
    
  #endif

}

/*
 * The CatMouse simulation will call this function each time a cat finishes
 * eating.
 *
 * You can use this function to wake up other creatures that may have been
 * waiting to eat until this cat finished.
 *
 * parameter: the number of the bowl at which the cat is finishing eating.
 *             legal bowl numbers are 1..NumBowls
 *
 * return value: none
 */

void
cat_after_eating(unsigned int bowl) 
{
  /* replace this default implementation with your own implementation of cat_after_eating */
  (void)bowl;  /* keep the compiler from complaining about an unused parameter */
  KASSERT(globalCatMouseSem != NULL);
  V(globalCatMouseSem);

  #if opt-A1
    V(bowls_sem);
    lock_acquire(all_bowls_lock);
    bowl_waiting[CAT][bowl]--;
    species_counter[CAT]--;

    // last of its species
    if(species_counter[CAT] == 0) {
      SPECIES = NONE;
      cv_broadcast(&cats_eating_cv, &all_bowls_lock);
      // cv_broadcast(&mice_eating_cv, &all_bowls_lock);
    }
    // other cats still eating
    else {
      cv_broadcast(&bowl_occupied_cv, &bowls_lock[bowl]);
    }

    lock_release(all_bowls_lock);
  #endif

}

/*
 * The CatMouse simulation will call this function each time a mouse wants
 * to eat, before it eats.
 * This function should cause the calling thread (a mouse simulation thread)
 * to block until it is OK for a mouse to eat at the specified bowl.
 *
 * parameter: the number of the bowl at which the mouse is trying to eat
 *             legal bowl numbers are 1..NumBowls
 *
 * return value: none
 */

void
mouse_before_eating(unsigned int bowl) 
{
  /* replace this default implementation with your own implementation of mouse_before_eating */
  (void)bowl;  /* keep the compiler from complaining about an unused parameter */
  KASSERT(globalCatMouseSem != NULL);
  P(globalCatMouseSem);



  #if opt-A1
    // down semaphore on bowls available
    KASSERT(bowls_sem != NULL);
    P(bowls_sem);
    lock_acquire(all_bowls_lock);

    // no one is currently eating
    if (SPECIES == NONE) {
      SPECIES = MOUSE;
      KASSERT(species_counter[MOUSE] == 0);
    }

    // mice currenly eating
    else if (SPECIES == MOUSE) {
      // wait while other mice are eating desired bowl
      while(SPECIES == MOUSE && bowl_waiting[MOUSE][bowl] > 0) {
        cv_wait(&bowl_occupied_cv[bowl], &bowls_lock[bowl]);
        // cv_wait(&mice_eating_cv, &bowls_lock[bowl]);
      }
    }

    // cats currently eating
    else {
      while(SPECIES == CAT) {
        // cv_wait(&bowl_occupied_cv[bowl], &bowls_lock[bowl]);
        cv_wait(&cats_eating_cv, &bowls_lock[bowl]);
      }
      // cats done eating
      if (SPECIES != CAT) {
        lock_acquire(all_bowls_lock);
        // SPECIES = MOUSE;
        SPECIES = NONE;
        KASSERT(species_counter[MOUSE] == 0);
      }
      // bowl_waiting[CAT][bowl]++;

    }

    // increment number of cats waiting for bowl
    bowl_waiting[MOUSE][bowl]++;
    species_counter[MOUSE]++;

    lock_release(bowls_lock);
    
  #endif
}

/*
 * The CatMouse simulation will call this function each time a mouse finishes
 * eating.
 *
 * You can use this function to wake up other creatures that may have been
 * waiting to eat until this mouse finished.
 *
 * parameter: the number of the bowl at which the mouse is finishing eating.
 *             legal bowl numbers are 1..NumBowls
 *
 * return value: none
 */

void
mouse_after_eating(unsigned int bowl) 
{
  /* replace this default implementation with your own implementation of mouse_after_eating */
  (void)bowl;  /* keep the compiler from complaining about an unused parameter */
  KASSERT(globalCatMouseSem != NULL);
  V(globalCatMouseSem);

  #if opt-A1
    V(bowls_sem);
    lock_acquire(all_bowls_lock);
    bowl_waiting[MOUSE][bowl]--;
    species_counter[MOUSE]--;

    // last of its species
    if(species_counter[CAT] == 0) {
      SPECIES = NONE;
      // cv_broadcast(&cats_eating_cv, &all_bowls_lock);
      cv_broadcast(&mice_eating_cv, &all_bowls_lock);
    }
    // other mice still eating
    else {
      cv_broadcast(&bowl_occupied_cv, &bowls_lock[bowl]);
    }

    lock_release(all_bowls_lock);
  #endif
}
