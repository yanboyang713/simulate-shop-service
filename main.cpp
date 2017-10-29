/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   Student ’s Name : Boyang YAN
   Student ’s email address : by932@uowmail.edu.au
   Student user name : by932
   Assignment : Assignment two
   About : Shop Service Simulator
   Last Modification : 02/10/2017
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
#include <iostream>
#include <fstream>
#include <iomanip>      // std::setprecision
using namespace std;
#define BIGNUM 100000
#define MAXQUEUELENGTH 100
struct server
{
    double priority;
    int customersServed;
    double busyTime;
    bool isIdle;
};
server * serverPool;//for story all of server status

struct event
{
    int eventType;//-1 mean customer arriving event. others number mean which server service this customer
    double eventTime;
    double serviceTime;
};
event * eventPriorityQueue;//for management what is next event, top element will be first finish. represent in heap
int eventPriorityQueueIndex = 0;//this is for index the last element in eventPriorityQueue

//in the FIFO queue need record arrivalTime and serviceTime for each customer
struct queueElement
{
    double arrivalTime;
    double serviceTime;
};
//firest in first out queue for story writing queue
queueElement FIFOqueue[MAXQUEUELENGTH];
int queueHeadIndex = 0;//this is for index the begin FIFOqueue
int queueEndIndex = 0;//this is for index the end of FIFOqueue
int greatestLengthReachedByQueue = 0;
//below two for calculate average length of the queue
int queueLengthChangeTime = 0;
int queueLengthEachTimeChange[BIGNUM];
double averageLengthQueue = 0.00;

int totalCustomerServed = 0;
int numberOfServer = 0;
//below two for calculate average customer waiting time
int numberOfCustomerWaitingInQueue = 0;
double customerWaitingTimeInQueue[BIGNUM];
double currentTime = 0.0;

int findTheFastestIdleServer();
void readCustomerArriveEventAndInsertToEventPriorotyQueue(ifstream &);
queueElement FIFOqueuePop();
void FIFOqueueInsert( double &, double &);
void siftUp(int);
int parent(int);
void swap(int, int);
void removeByIndex(int);
void siftDown(int);
int leftChild(int);
int rightChild(int);
void insert(event &);
void priorityQueueAssign(int &, event &);

void creatCustomerCompletePaymentEventAddToPriorityQueue(int &, double &);

//quicksort
void quickSort (int, int);
int partition (int, int);

int getCurrentQueueLength();

int main(int argc, char *argv[])
{
    char fileName [20];
    cout << "Please input file name for open : ";
    cin >> fileName;
    // open file
    ifstream file (fileName);
    if ( file.is_open() ) {
        file >> numberOfServer;
        if (file.good()){
            serverPool = new server[numberOfServer];
            eventPriorityQueue = new event [numberOfServer + 1];
        }
        //read server priority to server pool
        for (int i = 0; i < numberOfServer; ++i) {
            file >> serverPool[i].priority;
            serverPool[i].customersServed = 0;
            serverPool[i].busyTime = 0.0;
            serverPool[i].isIdle = true;
        }
        //using quickSort sort serverPool according priority for perpare binary search find the idle and faster server
        quickSort(0, numberOfServer);

        //Read 1st CustomerArrival event from file and add it to the priorityEventHeap
        readCustomerArriveEventAndInsertToEventPriorotyQueue(file);
        //eventPriorityQueueIndex != -1 mean event priority not empty
        while (eventPriorityQueueIndex != 0) {
            event nextEvent = eventPriorityQueue[0];
            currentTime = eventPriorityQueue[0].eventTime;
            //event is customer arrival event
            if (nextEvent.eventType == -1) {
                //add customer to FIFO queue if all of server are busy or insert to priority queue
                FIFOqueueInsert(nextEvent.eventTime, nextEvent.serviceTime);

                //delete the top element in eventPriorityQueue
                removeByIndex(0);

                //read next Customer Arrive Event if not EOF Insert To Event Prioroty Queue
                readCustomerArriveEventAndInsertToEventPriorotyQueue(file);
            }
            else {//must be a customer complete payment event
                serverPool[nextEvent.eventType].isIdle = true;

                //record how many customer be served in this server
                serverPool[nextEvent.eventType].customersServed++;

                removeByIndex(0);//delete the top element in eventPriorityQueue

                totalCustomerServed++;
                }


            //find the fastest idle server index, if return -1 mean no idle server
            int fasterIndex = findTheFastestIdleServer();

            //queueEndIndex != queueHeadIndex mean FIFO not empty. fasterIndex != -1 mean idle server available
            if (queueEndIndex != queueHeadIndex && fasterIndex != -1){
                queueElement nextCustomer = FIFOqueuePop();//get next customer from FIFO queue

                creatCustomerCompletePaymentEventAddToPriorityQueue(fasterIndex, nextCustomer.serviceTime);

                //average customer waiting time
                customerWaitingTimeInQueue[numberOfCustomerWaitingInQueue++] = currentTime - nextCustomer.arrivalTime;
            }
        }
    }
    //print stats
    cout << "Number of customers served: " << totalCustomerServed << endl;
    cout << "Time last customer completed service: " << currentTime << endl;
    cout << "Greatest length reached by the queue: " << greatestLengthReachedByQueue << endl;

    //calculate average Length of Queue
    int queueLengthChangeSum = 0;
    for (int i = 0; i < queueLengthChangeTime; ++i)
        queueLengthChangeSum += queueLengthEachTimeChange[i];
    averageLengthQueue = static_cast<double>(queueLengthChangeSum) / (totalCustomerServed * 2);
    cout << "Average length of the queue: " << averageLengthQueue << endl;

    //calculate average Customer Waiting Time
    double averageCustomerWaitingTime = 0.00;
    double customerWaitingTimeSum = 0.00;
    for (int i = 0; i < numberOfCustomerWaitingInQueue; ++i)
        customerWaitingTimeSum += customerWaitingTimeInQueue[i];
    averageCustomerWaitingTime = customerWaitingTimeSum / totalCustomerServed;
    cout << "Average customer waiting time: " << averageCustomerWaitingTime << endl;
    std::cout.precision(4);
    std::cout.setf( std::ios::fixed, std::ios::floatfield );

    //print server stat
    std::cout << "Checkout" << std::setw(10) << "Priority" << std::setw(20) << "CustomersServed" << std::setw(15) << "IdleTime" << endl;
    for (int i = 0; i < numberOfServer; ++i) {
        std::cout << std::setw(4) << i;
        std::cout << std::setw(13) << serverPool[i].priority;
        std::cout << std::setw(15) << serverPool[i].customersServed;
        std::cout << std::setw(20) << currentTime - serverPool[i].busyTime << endl;
    }

    delete [] serverPool;
    delete [] eventPriorityQueue;
    return 0;
}
//using in quickSort
int partition (int left, int right)
{
    int val = left;
    int lm = left - 1;
    int rm = right + 1;
    for (;;)
    {
        do
        {
            rm--;
        }while (serverPool[rm].priority < serverPool[val].priority);//rm < val
        do
        {
            lm++;
        }while (serverPool[lm].priority > serverPool[val].priority);//lm > val
        if (lm < rm){
            //swap
            server tempr = serverPool[rm];
            serverPool[rm] = serverPool[lm];
            serverPool[lm] = tempr;
        }
        else
            return rm;
    }
}
//quickSort for sort server by priority. the fastest at end end
void quickSort (int left, int right)
{
    if (left < right){
        int splitPt = partition(left, right);
        quickSort(left, splitPt);
        quickSort(splitPt + 1, right);
    }
    return;
}
//this will return the faster idle server index. if return -1 mean no idle server
int findTheFastestIdleServer(){
    for (int i = numberOfServer - 1; i >= 0; i--) {
        if (serverPool[i].isIdle == true) {
            return i;
        }
    }
    return -1;//-1 mean all of server busy
}
void readCustomerArriveEventAndInsertToEventPriorotyQueue(ifstream &file){
    double readArrivalTime = 0.0;
    double readServiceTime = 0.0;
    file >> readArrivalTime;
    file >> readServiceTime;
    if (file.good()) {
        event insertData;
        insertData.eventType = -1;
        insertData.eventTime = readArrivalTime;
        insertData.serviceTime = readServiceTime;

        insert(insertData);//insert to priority queue
    }
    return;
}
queueElement FIFOqueuePop(){
    int currentQueueLength = getCurrentQueueLength() - 1;
    queueLengthEachTimeChange[queueLengthChangeTime++] = currentQueueLength;

    if (queueHeadIndex + 1 == MAXQUEUELENGTH) {
        queueHeadIndex = 0;
        return FIFOqueue[MAXQUEUELENGTH - 1];
    }
    else {
        queueElement out;
        out.arrivalTime = FIFOqueue[queueHeadIndex].arrivalTime;
        out.serviceTime = FIFOqueue[queueHeadIndex].serviceTime;
        queueHeadIndex++;
        return out;
    }
}
void FIFOqueueInsert(double & arrivalTimeIn, double & serviceTimeIn){
    int index = findTheFastestIdleServer();
    //findTheFastestIdleServer == -1 mean all server are busy to insert to FIFO queue
    if (index == -1) {
        if (queueEndIndex + 1 == MAXQUEUELENGTH) {
            FIFOqueue[MAXQUEUELENGTH - 1].arrivalTime = arrivalTimeIn;
            FIFOqueue[MAXQUEUELENGTH - 1].serviceTime = serviceTimeIn;
            queueEndIndex = 0;
        }
        else {
            FIFOqueue[queueEndIndex].arrivalTime = arrivalTimeIn;
            FIFOqueue[queueEndIndex].serviceTime = serviceTimeIn;
            queueEndIndex++;
        }
        //calculate the greatestLengthReachedByQueue
        int currentQueueLength = getCurrentQueueLength();
        //if currentQueueLength bigger than greatestLengthReachedByQueue, set the new greatestLengthReachedByQueue
        if (greatestLengthReachedByQueue < currentQueueLength)
            greatestLengthReachedByQueue = currentQueueLength;

        queueLengthEachTimeChange[queueLengthChangeTime++] = currentQueueLength;
    }else {
        creatCustomerCompletePaymentEventAddToPriorityQueue(index, serviceTimeIn);
    }
    return;
}
int getCurrentQueueLength(){
    int currentQueueLength = 0;
    //calculate current queue length
    if (queueHeadIndex < queueEndIndex)
        currentQueueLength = queueEndIndex - queueHeadIndex;
    else if (queueHeadIndex == queueEndIndex)
        currentQueueLength = 0;
    else
        currentQueueLength = MAXQUEUELENGTH - queueHeadIndex + queueEndIndex;
    return currentQueueLength;
}
void creatCustomerCompletePaymentEventAddToPriorityQueue(int & serviceServer,double & serviceTimeIn){
    serverPool[serviceServer].isIdle = false;//set server's idle flag to busy

    event customerCompetePaymentEvent;
    customerCompetePaymentEvent.eventType = serviceServer;
    customerCompetePaymentEvent.eventTime  = serviceTimeIn * serverPool[serviceServer].priority + currentTime;
    customerCompetePaymentEvent.serviceTime = serviceTimeIn * serverPool[serviceServer].priority;

    //set this server busy time
    serverPool[serviceServer].busyTime += customerCompetePaymentEvent.serviceTime;

    insert(customerCompetePaymentEvent);
    return;
}
//siftUp eventPriorityQueue
void siftUp(int indexIn)
{
    int parentIdx = parent(indexIn);
    if (parentIdx == -1)//index is roof
        return;
    if (eventPriorityQueue[indexIn].eventTime < eventPriorityQueue[parentIdx].eventTime) {
        swap(parentIdx, indexIn);
        siftUp(parentIdx);
    }
    return;
}
int parent(int index)
{
    index++;
    if (index <= 1)//empty or root has no parent
        return -1;

    return (index / 2) - 1;
}
void swap(int indexOne, int indexTwo)
{
    event temp;
    temp.eventType = eventPriorityQueue[indexOne].eventType;
    temp.eventTime = eventPriorityQueue[indexOne].eventTime;
    temp.serviceTime = eventPriorityQueue[indexOne].serviceTime;

    eventPriorityQueue[indexOne].eventType = eventPriorityQueue[indexTwo].eventType;
    eventPriorityQueue[indexOne].eventTime = eventPriorityQueue[indexTwo].eventTime;
    eventPriorityQueue[indexOne].serviceTime = eventPriorityQueue[indexTwo].serviceTime;

    eventPriorityQueue[indexTwo].eventType = temp.eventType;
    eventPriorityQueue[indexTwo].eventTime = temp.eventTime;
    eventPriorityQueue[indexTwo].serviceTime = temp.serviceTime;
    return;
}
void removeByIndex(int indexIn)
{
    if (indexIn + 1 > eventPriorityQueueIndex) {
        cout << "Error: this index does not exist :(" << endl;
        return;
    }
    swap(indexIn, --eventPriorityQueueIndex); //swap current with last item
    siftDown(indexIn);
    return;
}
void siftDown(int indexIn)
{
    int leftChildIndex = leftChild(indexIn);
    int rightChildIndex = rightChild(indexIn);
    int childIndex = -1;
    if (leftChildIndex == -1) {
        if (rightChildIndex == -1)
            return;//no right and left child
        else
            childIndex = rightChildIndex;
    }else {
        if (rightChildIndex == -1)
            childIndex = leftChildIndex;
        else {
            if (eventPriorityQueue[rightChildIndex].eventTime <= eventPriorityQueue[leftChildIndex].eventTime)
                childIndex = rightChildIndex;
            else
                childIndex = leftChildIndex;
        }
    }

    if (eventPriorityQueue[indexIn].eventTime > eventPriorityQueue[childIndex].eventTime)
    {
        swap(indexIn, childIndex);
        siftDown(childIndex);
    }
    return;

}
int leftChild(int indexIn)//left child
{
    indexIn++;

    if (eventPriorityQueueIndex == 0 || 2 * indexIn > eventPriorityQueueIndex )//empty or root has no child
        return -1;

    return (2 * indexIn) - 1;
}
int rightChild(int indexIn){
    indexIn++;
    if (eventPriorityQueueIndex == 0 || 2 * indexIn + 1 > eventPriorityQueueIndex )//empty or root has no child
        return -1;

    return 2 * indexIn;
}
void insert(event & val)
{
    priorityQueueAssign(eventPriorityQueueIndex, val);
    siftUp(eventPriorityQueueIndex++);
    return;
}
//assign a event to priority queue. index is assign to which element, eventin is assign data
void priorityQueueAssign(int & index, event & eventIn){
    eventPriorityQueue[index].eventType = eventIn.eventType;
    eventPriorityQueue[index].eventTime = eventIn.eventTime;
    eventPriorityQueue[index].serviceTime = eventIn.serviceTime;
    return;
}
