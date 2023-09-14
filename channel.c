// Sai Narayan 
// CMPSC 473 - Concurrencylab

#include "channel.h"

struct SubscriberNode {                                     // Struct to store the subscriber node
  int signalFlag;
  pthread_mutex_t mutex;                                    // Mutex for subscriber node
  pthread_cond_t cond;                                      // Condition variable for subscriber node
};

typedef struct SubscriberNode Subscriber;                   // Typedef for subscriber node

static void set_signal_flag(Subscriber* subscriberPtr) {    // Set signal flag for subscriber node
    pthread_mutex_lock(&subscriberPtr->mutex);              // Lock subscriber node mutex
    subscriberPtr->signalFlag = 1;                          // Set signal flag to 1
    pthread_mutex_unlock(&subscriberPtr->mutex);            // Unlock subscriber node mutex
}

static void signal_condition(Subscriber* subscriberPtr) {       // Signal subscriber condition variable
    pthread_mutex_lock(&subscriberPtr->mutex);                  //  Lock subscriber node mutex
    pthread_cond_signal(&subscriberPtr->cond);                  //  Signal subscriber condition variable
    pthread_mutex_unlock(&subscriberPtr->mutex);                //  Unlock subscriber node mutex
}

static void SignalSubscriber(void* data) {                     // Inform (unblock) a subscriber
    Subscriber* subscriberPtr = (Subscriber*)data;             // Cast data to subscriber node
    set_signal_flag(subscriberPtr);                            // Set signal flag      
    signal_condition(subscriberPtr);                           // Signal subscriber condition variable
}

static void subscriber_wait(Subscriber *subscriberPtr) {
 
  pthread_mutex_lock(&subscriberPtr->mutex);                         // Lock the mutex    

  // Wait until an event on list
  do {
    pthread_cond_wait(&subscriberPtr->cond, &subscriberPtr->mutex);   // Wait on condition variable
  } while(subscriberPtr->signalFlag == 0);                            

  pthread_mutex_unlock(&subscriberPtr->mutex);                       // Unlock the mutex
}

// Creates a new channel with the provided size and returns it to the caller
// A 0 size indicates an unbuffered channel, whereas a positive size indicates a buffered channel

static Subscriber* subscriber_create(select_t *channel_list, size_t channel_count) {
   
    Subscriber *subscriberPtr = (Subscriber*) calloc(1, sizeof(Subscriber));      // Allocate memory for subscriber node
    if (!subscriberPtr) return NULL;                                              // Return NULL if allocation fails  

    if (pthread_mutex_init(&subscriberPtr->mutex, NULL) != 0) {                   // Initialize mutex
        free(subscriberPtr);                                                      // Free subscriber node if mutex initialization fails
        return NULL;                                                              // Return NULL if mutex initialization fails
    }
    if (pthread_cond_init(&subscriberPtr->cond, NULL) != 0) {                     // Initialize condition variable
        pthread_mutex_destroy(&subscriberPtr->mutex);                             // Destroy mutex if condition variable initialization fails
        free(subscriberPtr);                                                      // Free subscriber node if condition variable initialization fails
        return NULL;
    }

  // Iterate over each channel

    for (size_t i = 0; i < channel_count; ++i) {                      
        select_t *sel = &channel_list[i];
        if (pthread_mutex_lock(&sel->channel->mutex) != 0) continue;              // Lock the mutex before modifying the channel
        list_insert(sel->channel->subscribers, (void*)subscriberPtr);             // Insert subscriber node into the list
        pthread_mutex_unlock(&sel->channel->mutex);                               // Unlock the mutex after modifying the channel
    }

    return subscriberPtr;
}

static void subscriber_destroy(select_t * channel_list, size_t channel_count, Subscriber * subscriberPtr){
  
  for(size_t i = 0; i < channel_count; ++i){
    select_t * sel = &channel_list[i];
    pthread_mutex_lock(&sel->channel->mutex);                               //  Lock the mutex before modifying the channel
    list_node_t* node = list_find(sel->channel->subscribers, (void*)subscriberPtr);     // Find subscriber node in the list
    
    if(node != NULL){                                                       // If subscriber node is found in the list
      list_remove(sel->channel->subscribers, node);                         // Remove subscriber node from the list
      free(node);
    }
    
    pthread_mutex_unlock(&sel->channel->mutex);                             // Unlock the mutex after modifying the channel
  }
  
  pthread_mutex_destroy(&subscriberPtr->mutex);                           //  Destroy mutex
  pthread_cond_destroy(&subscriberPtr->cond);                             //  Destroy condition variable
  free(subscriberPtr);                                                    //  Free subscriber node    
}

channel_t* channel_create(size_t size)                                  
{
    if(size <= 0){                                                  // If size is less than or equal to 0
        fprintf(stderr, "Invalid size\n");                          // Print error message
      exit(1);    
    }

    channel_t* channel = (channel_t*) malloc(sizeof(channel_t));          // Allocate memory for channel
    

    if (!channel || 
        !(channel->buffer = buffer_create(size)) ||                               // Allocate memory for buffer
        pthread_mutex_init(&(channel->mutex), NULL) != 0 ||                       // Initialize mutex
        pthread_cond_init(&(channel->cond), NULL) != 0 ||                         // Initialize condition variable
        !(channel->subscribers = list_create()))                                // Create list                    
    {
        if(!channel) perror("malloc");
        else if(!(channel->buffer)) perror("buffer_create");               
        else if(pthread_mutex_init(&(channel->mutex), NULL) != 0) perror("pthread_mutex_init");     
        else if(pthread_cond_init(&(channel->cond), NULL) != 0) perror("pthread_cond_init");
        else perror("list_create");                                             // Print error message

        if(channel) {
            if(channel->buffer) free(channel->buffer);                          //  Free buffer
            free(channel);
        }

        exit(EXIT_FAILURE);
    }

    channel->end_flag = 0;                            // Set end flag to 0  

    return channel; 
}

// Writes data to the given channel
// This is a blocking call i.e., the function only returns on a successful completion of send
// In case the channel is full, the function waits till the channel has space to write the new data
// Returns SUCCESS for successfully writing data to the channel,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort

enum channel_status channel_send(channel_t *channel, void* data) {      
    if (pthread_mutex_lock(&channel->mutex) != 0) {     // Lock the mutex before modifying the channel
        return GEN_ERROR;
    }

    if (channel->end_flag) {                                                // If channel is closed
        pthread_mutex_unlock(&channel->mutex);                              // Unlock the mutex after modifying the channel
        return CLOSED_ERROR;
    }

    while (buffer_current_size(channel->buffer) == buffer_capacity(channel->buffer) && !channel->end_flag) {
        pthread_cond_wait(&channel->cond, &channel->mutex);                             // Wait on condition variable
    }

    if (channel->end_flag) {                                                //  If channel is closed
        pthread_mutex_unlock(&channel->mutex);                              // Unlock the mutex after modifying the channel
        return CLOSED_ERROR;                                                // Return CLOSED_ERROR if channel is closed
    }

    enum channel_status sn = buffer_add(channel->buffer, data) == BUFFER_SUCCESS ? SUCCESS : GEN_ERROR;     // Add data to buffer

    if (sn == SUCCESS) {
        pthread_cond_broadcast(&channel->cond);                                   // Signal all threads waiting on condition variable
        list_foreach(channel->subscribers, SignalSubscriber);                     // Signal all subscribers
    }

    pthread_mutex_unlock(&channel->mutex);                                        //  Unlock the mutex after modifying the channel

    return sn;
}


// Reads data from the given channel and stores it in the function's input parameter, data (Note that it is a double pointer)
// This is a blocking call i.e., the function only returns on a successful completion of receive
// In case the channel is empty, the function waits till the channel has some data to read
// Returns SUCCESS for successful retrieval of data,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort

enum channel_status channel_receive(channel_t* channel, void** data)
{
  enum channel_status rv = SUCCESS;
  int lockStatus = pthread_mutex_lock(&channel->mutex);        // lock the channel

  if(lockStatus != 0){                                         // check if the lock was successful
    return GEN_ERROR;                                          // return error if lock failed 
  }

  do{
    if(channel->end_flag){                                     // check if the channel is closed
      rv = CLOSED_ERROR;                                       // return error, if channel is closed
      break;                                                   // break from the loop
    }

    //wait while the buffer is empty
    while(  (buffer_current_size(channel->buffer) == 0) &&     // check if the buffer is empty
            (channel->end_flag == 0) ){                        // check if the channel is closed
      pthread_cond_wait(&channel->cond, &channel->mutex);      // wait for a signal that the buffer is not empty
    }

    if(channel->end_flag){                                     // check if the channel is closed
      rv = CLOSED_ERROR;                                       // return error, if channel is closed
      break;                                                   // break from the loop
    }

    if(buffer_remove(channel->buffer, data) != BUFFER_SUCCESS){ //get data from channel
      rv = GEN_ERROR;                                           //return error, if remove failed
      break;
    }

    pthread_cond_broadcast(&channel->cond);                    // tell waiting threads data was removed

    list_foreach(channel->subscribers, SignalSubscriber);      // tell each subscriber we have event on this list

  }while(0);                                                   // end of do-while loop

  pthread_mutex_unlock(&channel->mutex);                       // unlock the mutex before signalling the subscribers

  return rv;                                                   // return the status
}


// Writes data to the given channel
// This is a non-blocking call i.e., the function simply returns if the channel is full
// Returns SUCCESS for successfully writing data to the channel,
// CHANNEL_FULL if the channel is full and the data was not added to the buffer,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort

enum channel_status channel_non_blocking_send(channel_t* channel, void* data) {
  
    // Attempt to lock the channel. On failure, return general error
    if(pthread_mutex_lock(&channel->mutex) != 0){                       // lock the channel
        return GEN_ERROR;                                               // return error if lock failed
    }

    // If the channel is closed, unlock the mutex and return closed error
    if(channel->end_flag){                                              // check if the channel is closed
        pthread_mutex_unlock(&channel->mutex);                          // unlock the mutex before signalling the subscribers
        return CLOSED_ERROR;                                            // return error, if channel is closed
    }

    // If the buffer is full, unlock the mutex and return channel full error
    if(buffer_current_size(channel->buffer) == buffer_capacity(channel->buffer)){  // check if the buffer is full
        pthread_mutex_unlock(&channel->mutex);                                     // unlock the mutex before signalling the subscribers
        return CHANNEL_FULL;                                                       // return error, if channel is full
    }

    // Try to add data to buffer, on failure unlock mutex and return general error
    if(buffer_add(channel->buffer, data) != BUFFER_SUCCESS){                       // add data to channel
        pthread_mutex_unlock(&channel->mutex);                                     // unlock the mutex before signalling the subscribers
        return GEN_ERROR;                                                          // return error, if add failed
    }

    // If data added successfully, broadcast condition variable and signal subscribers
    pthread_cond_broadcast(&channel->cond);                             // tell waiting threads data was inserted
    list_foreach(channel->subscribers, SignalSubscriber);               // tell each subscriber we have event on this list

    // Finally unlock the mutex and return success
    pthread_mutex_unlock(&channel->mutex);                              // unlock the mutex before signalling the subscribers

    return SUCCESS;
}

// Reads data from the given channel and stores it in the function's input parameter data (Note that it is a double pointer)
// This is a non-blocking call i.e., the function simply returns if the channel is empty
// Returns SUCCESS for successful retrieval of data,
// CHANNEL_EMPTY if the channel is empty and nothing was stored in data,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort

enum channel_status handleError(pthread_mutex_t* mutex, enum channel_status error) {   // Function to handle errors
    pthread_mutex_unlock(mutex);                                                       // Unlock the mutex
    return error;
}

enum channel_status handleSuccess(pthread_mutex_t* mutex, pthread_cond_t* cond, list_t* subscribers) {      // Function to handle success
    pthread_cond_broadcast(cond);                                                                           // Broadcast the condition variable
    list_foreach(subscribers, SignalSubscriber);                                                            // Signal the subscribers
    pthread_mutex_unlock(mutex);                                                                            // Unlock the mutex
    return SUCCESS;                                                                                         // Return success
}

enum channel_status channel_non_blocking_receive(channel_t* channel, void** data)
{
    if (pthread_mutex_lock(&channel->mutex) != 0){                                                          // Lock the mutex
        return GEN_ERROR;
    }

    if(channel->end_flag){
        return handleError(&channel->mutex, CLOSED_ERROR);                                                            // Return error if channel is closed
    } else if(buffer_current_size(channel->buffer) == 0){
        return handleError(&channel->mutex, CHANNEL_EMPTY);                                                    //  Return error if channel is empty
    } else {
        enum buffer_status buffer_status = buffer_remove(channel->buffer, data);                           // Remove data from buffer
        if (buffer_status != BUFFER_SUCCESS) {
            return handleError(&channel->mutex, GEN_ERROR);                                                // Return error if data was not removed successfully
        } else {                                                                                           // If data was removed successfully 
            return handleSuccess(&channel->mutex, &channel->cond, channel->subscribers);                   // Handle success
        }
    }
}

// Closes the channel and informs all the blocking send/receive/select calls to return with CLOSED_ERROR
// Once the channel is closed, send/receive/select operations will cease to function and just return CLOSED_ERROR
// Returns SUCCESS if close is successful,
// CLOSED_ERROR if the channel is already closed, and
// GEN_ERROR in any other error case

enum channel_status channel_close(channel_t* channel)                                          
{
    if (pthread_mutex_lock(&channel->mutex) != 0) {                                                 // Lock the mutex
        return GEN_ERROR;                                                                           // Return error if lock failed
    }

    if (channel->end_flag == 1) {                                                                   // Check if the channel is already closed
        pthread_mutex_unlock(&channel->mutex);                                                      // Unlock the mutex
        return CLOSED_ERROR;                                                                        // Return error if channel is already closed
    }

    channel->end_flag = 1;                                                                          // Set the end flag to 1

    list_foreach(channel->subscribers, SignalSubscriber);                                           // Signal the subscribers

    pthread_cond_broadcast(&channel->cond);                                                         // Broadcast the condition variable

    pthread_mutex_unlock(&channel->mutex);                                                          // Unlock the mutex

    return SUCCESS;
}

// Frees all the memory allocated to the channel
// The caller is responsible for calling channel_close and waiting for all threads to finish their tasks before calling channel_destroy
// Returns SUCCESS if destroy is successful,
// DESTROY_ERROR if channel_destroy is called on an open channel, and
// GEN_ERROR in any other error case

enum channel_status channel_destroy(channel_t* channel)                                     // Function to destroy the channel
{
    bool channel_not_closed;                                                                // Boolean to check if the channel is closed
    
    pthread_mutex_lock(&channel->mutex);                                                    // Lock the mutex
    channel_not_closed = (channel->end_flag == 0);                                          // Check if the channel is closed
    pthread_mutex_unlock(&channel->mutex);                                                  // Unlock the mutex

    if (channel_not_closed) {                                                               // Return error if channel is not closed
        return DESTROY_ERROR;                                                               // Return error if channel is not closed
    }

    // cleanup synchronization primitives
    pthread_cond_destroy(&channel->cond);                                                   // Destroy the condition variable
    pthread_mutex_destroy(&channel->mutex);                                                 // Destroy the mutex

    // free resources associated with the channel
    buffer_free(channel->buffer);                                                           //  Free the buffer
    list_destroy(channel->subscribers);                                                     // Destroy the list of subscribers
    
    // deallocate the channel
    free(channel);                                                                          // Free the channel

    return SUCCESS;
}

// Takes an array of channels (channel_list) of type select_t and the array length (channel_count) as inputs
// This API iterates over the provided list and finds the set of possible channels which can be used to invoke the required operation (send or receive) specified in select_t
// If multiple options are available, it selects the first option and performs its corresponding action
// If no channel is available, the call is blocked and waits till it finds a channel which supports its required operation
// Once an operation has been successfully performed, select should set selected_index to the index of the channel that performed the operation and then return SUCCESS
// In the event that a channel is closed or encounters any error, the error should be propagated and returned through select
// Additionally, selected_index is set to the index of the channel that generated the error
enum channel_status channel_select(select_t* channel_list, size_t channel_count, size_t* selected_index)
{
    size_t i;
    Subscriber* subscriberPtr = NULL;   // Pointer to subscriber object
    enum channel_status sn = SUCCESS;   // Assume success 

    *selected_index = channel_count;    // Set selected index to an invalid value initially

    do {
        for (i = 0; i < channel_count; i++) {               // Iterate over the channel list
            select_t* sel = &channel_list[i];                                 
            enum channel_status result;                     // Variable to store the result of the operation
            if (sel->dir == SEND) {
                result = channel_non_blocking_send(sel->channel, sel->data);            // Perform the operation
            } else {
                result = channel_non_blocking_receive(sel->channel, &sel->data);                // Perform the operation
            }   

            if (result == SUCCESS || result == CLOSED_ERROR) {                          //  If the operation was successful or the channel is closed
                *selected_index = i;                // Set the selected index   
                sn = result;            // Set the status   
                break;                                              
            }
        }

        if (*selected_index == channel_count) {                              // If no channel was selected
            if (subscriberPtr == NULL) {                                     // If the subscriber object is not created
                subscriberPtr = subscriber_create(channel_list, channel_count);             // Create a subscriber object
                if (subscriberPtr == NULL) {                                            // If the subscriber object was not created successfully
                    return GEN_ERROR;
                }
            } else {
                subscriber_wait(subscriberPtr);                                     // Wait for the subscriber object
            }
        }
    } while (*selected_index == channel_count);                                     // Loop until a channel is selected

    if (subscriberPtr) {
        subscriber_destroy(channel_list, channel_count, subscriberPtr);             // Destroy the subscriber object
    }

    return sn;
}
