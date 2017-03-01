#include <assert.h>
#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include <algorithm>
#include <numeric>
#include <vector>

#include "papi.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>
#define EVENT_NUM       3


// The value of argv[0] passed to main(). Used in error messages.
static const char* gArgv0;

// A special value that represents an estimate from an unsupported RAPL domain.
static const double kUnsupported_j = -1.0;

//----------------------------------------------------------------------------


static void
Abort(const char* aFormat, ...)
{
  va_list vargs;
  va_start(vargs, aFormat);
  fprintf(stderr, "%s: ", gArgv0);
  vfprintf(stderr, aFormat, vargs);
  fprintf(stderr, "\n");
  va_end(vargs);

  exit(1);
}

//---------------------------------------------------------------------------
// Linux-specific code
//---------------------------------------------------------------------------

#include <linux/perf_event.h>
#include <sys/syscall.h>

// There is no glibc wrapper for this system call so we provide our own.
static int
perf_event_open(struct perf_event_attr* aAttr, pid_t aPid, int aCpu,
                int aGroupFd, unsigned long aFlags)
{
  return syscall(__NR_perf_event_open, aAttr, aPid, aCpu, aGroupFd, aFlags);
}

// Returns false if the file cannot be opened.
template <typename T>
static bool
ReadValueFromPowerFile(const char* aStr1, const char* aStr2, const char* aStr3,
                       const char* aScanfString, T* aOut)
{
  // The filenames going into this buffer are under our control and the longest
  // one is "/sys/bus/event_source/devices/power/events/energy-cores.scale".
  // So 256 chars is plenty.
  char filename[256];

  sprintf(filename, "/sys/bus/event_source/devices/power/%s%s%s",
          aStr1, aStr2, aStr3);
  FILE* fp = fopen(filename, "r");
  if (!fp) {
    return false;
  }
  if (fscanf(fp, aScanfString, aOut) != 1) {
    Abort("fscanf() failed");
  }
  fclose(fp);

  return true;
}


//---------------------------------------------------------------------------------------------------------------------
// This class encapsulates the reading of a single RAPL domain.
class Domain
{
  bool mIsSupported;      // Is the domain supported by the processor?

  // These three are only set if |mIsSupported| is true.
  double mJoulesPerTick;  // How many Joules each tick of the MSR represents.
  int mFd;                // The fd through which the MSR is read.
  double mPrevTicks;      // The previous sample's MSR value.

public:
  enum IsOptional { Optional, NonOptional };

  Domain(const char* aName, uint32_t aType, IsOptional aOptional = NonOptional)
  {
    uint64_t config;
    if (!ReadValueFromPowerFile("events/energy-", aName, "", "event=%llx",
         &config)) {
      // Failure is allowed for optional domains.
      if (aOptional == NonOptional) {
        Abort("failed to open file for non-optional domain '%s'\n"
              "- Is your kernel version 3.14 or later, as required? "
              "Run |uname -r| to see.", aName);
      }
      mIsSupported = false;
      return;
    }

    mIsSupported = true;

    ReadValueFromPowerFile("events/energy-", aName, ".scale", "%lf",
                           &mJoulesPerTick);

    // The unit should be "Joules", so 128 chars should be plenty.
    char unit[128];
    ReadValueFromPowerFile("events/energy-", aName, ".unit", "%127s", unit);
    if (strcmp(unit, "Joules") != 0) {
      Abort("unexpected unit '%s' in .unit file", unit);
    }

    struct perf_event_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.type = aType;
    attr.size = uint32_t(sizeof(attr));
    attr.config = config;

    // Measure all processes/threads. The specified CPU doesn't matter.
    mFd = perf_event_open(&attr, /* pid = */ -1, /* cpu = */ 0,
                          /* group_fd = */ -1, /* flags = */ 0);
    if (mFd < 0) {
      Abort("perf_event_open() failed\n"
            "- Did you run as root (e.g. with |sudo|) or set\n"
            "  /proc/sys/kernel/perf_event_paranoid to 0, as required?");
    }

    mPrevTicks = 0;
  }

  ~Domain()
  {
    if (mIsSupported) {
      close(mFd);
    }
  }

  double EnergyEstimate()
  {
    if (!mIsSupported) {
      return kUnsupported_j;
    }

    uint64_t thisTicks;
    if (read(mFd, &thisTicks, sizeof(uint64_t)) != sizeof(uint64_t)) {
      Abort("read() failed");
    }

    uint64_t ticks = thisTicks - mPrevTicks;
    mPrevTicks = thisTicks;
    double joules = ticks * mJoulesPerTick;
    return joules;
  }

//wzx-----

  double readEnergy()
  {
	if(!mIsSupported){
		return -1.0;//KUnsupported_j;
	}
	uint64_t thisTicks;
	if(read(mFd,&thisTicks,sizeof(uint64_t)) != sizeof(uint64_t))
	{
		Abort("read() failed");
	}
	return thisTicks* mJoulesPerTick;
  }

//-------


};

class RAPL
{
  Domain* mPkg;
  Domain* mCores;
  Domain* mGpu;
  Domain* mRam;

public:
  RAPL()
  {
    uint32_t type;
    ReadValueFromPowerFile("type", "", "", "%u", &type);

    mPkg   = new Domain("pkg",   type);
    mCores = new Domain("cores", type);
    mGpu   = new Domain("gpu",   type, Domain::Optional);
    mRam   = new Domain("ram",   type, Domain::Optional);
    if (!mPkg || !mCores || !mGpu || !mRam) {
      Abort("new Domain() failed");
    }
  }

  ~RAPL()
  {
    delete mPkg;
    delete mCores;
    delete mGpu;
    delete mRam;
  }

  void EnergyEstimates(double& aPkg_J, double& aCores_J, double& aGpu_J,
                       double& aRam_J)
  {
    aPkg_J   = mPkg->EnergyEstimate();
    aCores_J = mCores->EnergyEstimate();
    aGpu_J   = mGpu->EnergyEstimate();
    aRam_J   = mRam->EnergyEstimate();
  }
//wzx----------------------------------------

	void TotalEnergy(double& aPkg_J,double& aCores_J, double& aGpu_J, double& aRam_J)
	{
		aPkg_J = mPkg->readEnergy();
		aCores_J = mCores->readEnergy();
		aGpu_J = mGpu->readEnergy();
		aRam_J = mRam->readEnergy();
	}
//--------------------------------------------------
};


//--------------------------------------------------------------------------------------------- 


// The sample interval, measured in seconds.
static double gSampleInterval_sec;

// The platform-specific RAPL-reading machinery.
static RAPL* gRapl;

// All the sampled "total" values, in Watts.
static std::vector<double> gTotals_W;

// Power = Energy / Time, where power is measured in Watts, Energy is measured
// in Joules, and Time is measured in seconds.
static double
JoulesToWatts(double aJoules)
{
  return aJoules / gSampleInterval_sec;
}


// "Normalize" here means convert kUnsupported_j to zero so it can be used in
// additive expressions. All printed values are 5 or maybe 6 chars (though 6
// chars would require a value > 100 W, which is unlikely).
static void
NormalizeAndPrintAsWatts(char* aBuf, double& aValue_J)
{
  if (aValue_J == kUnsupported_j) {
    aValue_J = 0;
    sprintf(aBuf, "%s", " n/a ");
  } else {
    sprintf(aBuf, "%5.2f", JoulesToWatts(aValue_J));
  }
}
//------------------------------------------------------------------------------------------------------------------------------
void *thread_utility(void *arg);//单线程，用于获取三种组件在采样周期内的利用率

int main() {



    char string[] ="sync && echo 3 > /proc/sys/vm/drop_caches && sleep 2 && echo 1 > /proc/sys/vm/drop_caches && echo 2 > /proc/sys/vm/drop_caches";
    system(string); //清除系统缓存

    int result;
    pthread_t a_thread;//用于获取组件利用率的单线程

    /* Initialize the PAPI library */
    if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT)
                exit(-1);
    

 	//create a thread used to deal with the performance sample stuff	
	result = pthread_create(&a_thread, NULL, thread_utility, NULL);//create a thread, return int type
    
	if(result != 0)
	{
	  perror("thread creation failed");
	  exit(1);
	}
	


    /* Do some computation here */
        int a = 0;
        int b = 1;
        for( int k = 0;k<9000000;k++)
        {
             for(int c =0;c < 3000;c++)
            {
                 a = rand()%100;
                  a = a*34.6;
                  }
            b = b+3;

         }


	//sleep(20);
        result = pthread_cancel(a_thread);
    	if(result != 0)
    	{
         	perror("Thread cancellation failed!\n");
         	exit(EXIT_FAILURE);
    	}

//    printf("waiting for thread to finish......\n");
       void *joinresult;
       result = pthread_join(a_thread,&joinresult);//等待回收其他线程，该函数u 会一直阻塞，直到被收回的线程结束为止
       if(result != 0)
       {
          exit(EXIT_FAILURE);
       }    
  
       exit(EXIT_SUCCESS);
}



void *thread_utility(void *arg)
{

//----------------------------------

	static int sampleNumer = 1;
	double pkg_J, cores_J, gpu_J, ram_J;
	double pwr = 0.0;
	double pwr1 ;
	double pwr2 ;

  // Initialize the platform-specific RAPL reading machinery.
  	gRapl = new RAPL();


//--------------------------------------
   int res;
   int EventSet;
   double interval;// 两次采样时间间隔
  // char msg[8];//接收服务端的消息
   int i=0;

   FILE *fp;
   struct timeval tv1;//前后两次采样时间值
   struct timeval tv2;
   struct tm* pt1;
   struct tm* pt2;
   time_t itime;
   int cur_millisec;//精确到毫秒级

    long_long values[EVENT_NUM] = {0};
    long_long values1[EVENT_NUM] = {0};
    long_long values2[EVENT_NUM] = {0};
    long_long values3[EVENT_NUM] = {0};
//----------------------------------------

	long_long evt = 0;
	long_long evt1 = 0;
	long_long evt2 = 0;
	long_long evt3 = 0;
	long_long evt4 = 0;
	long_long evt5 = 0;
	long_long evt6 = 0;
	long_long evt7 = 0;
	long_long evt8 = 0;
	long_long evt9 = 0;
	long_long evt10 = 0;
	long_long evt11 = 0;
	long_long evt12 = 0;
	long_long evt13 = 0;
	long_long evt14 = 0;

//-----------------------------------------
 


 //  float freq[4];
  // float curfreq[4];
   //int cpu = 0;
 //  float power;//功率估计值
   
   pthread_t child_thread;//子线程的id号
   int result,core;
   //cpu_set_t mask;
 
 
   res = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
                                                           
   if(res != 0)
   {
      perror("Thread pthread_setcancelstate failed!");
      exit(EXIT_FAILURE);
   }

   //PTHREAD_CANCEL_DEFERRED--接收到取消请求后，一直等待，直到线程执行一些函数
   res = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
                                                            
                                                             
   if(res != 0)
   {
      perror("Thread pthread_setcanceltype failed!");
      exit(EXIT_FAILURE);
   }
   
   if((fp = fopen("util-power.csv", "ab")) == NULL) // util-power.csv is used to store the performance statistics at certain time
   {
       perror("file open failed!");
       exit(EXIT_FAILURE);
   }

//   fprintf(fp,"timestamp,e0,e1,core2,core3,util0,util1,util2,util3,cpu,disk,predict,power\n");
   fprintf(fp,"timestamp,PAPI_L3_TCM,PAPI_L1_LDM,PAPI_L1_STM,PAPI_L2_LDM,PAPI_L2_STM,PAPI_PRF_DM,PAPI_BR_CN,PAPI_TOT_INS,PAPI_LD_INS,PAPI_LST_INS,PAPI_BR_INS,PAPI_TOT_CYC,PAPI_SR_INS,PAPI_MEM_WCY,PAPI_DP_OPS,power\n");

   //printf("thread function is running......\n");   
/*
   gettimeofday (&tv1, NULL);
   itime = time(NULL);//从1970年－1-1零点零分到当前系统所偏移的秒数
   pt1 = localtime(&itime);//将从1970－1-1零点零分到当前时间系统所偏移的秒数时间转换为本地时间
   cur_millisec = tv1.tv_usec/1000;
   
*/
//---------------------------------------------------

	gRapl->TotalEnergy(pkg_J,cores_J,gpu_J,ram_J);
  	// We should have pkg and cores estimates, but might not have gpu and ram
  	// estimates.

  	assert(pkg_J   != kUnsupported_j);    //assert计算表达式，0为假，并输出
  	assert(cores_J != kUnsupported_j);


 	// This needs to be big enough to print watt values to two decimal places. 16
  	// should be plenty.
  	//static const size_t kNumStrLen = 16;

  	//static char pkgStr[kNumStrLen], coresStr[kNumStrLen], gpuStr[kNumStrLen],
          //    ramStr[kNumStrLen];

  	// Core and GPU power are a subset of the package power.
  	assert(pkg_J >= cores_J + gpu_J);

  	// Compute "other" (i.e. rest of the package) and "total" only after the
  	// other values have been normalized.

  	//char otherStr[kNumStrLen];
  	double other_J = 0.0;
	other_J = pkg_J - cores_J - gpu_J;
  	//NormalizeAndPrintAsWatts(otherStr, other_J);

  	//char totalStr[kNumStrLen];
  	double total_J = 0.0;
	total_J = pkg_J + ram_J;
	//printf("%5.2f",total_J);
  	//NormalizeAndPrintAsWatts(totalStr, total_J);
	//char *pwr_first = (char*)malloc(strlen(totalStr));
	//strcpy(pwr_first,totalStr); 

//------------------------------------------------------

   int fd[256];//文件描述符
   
    /* Initialize the PAPI library */
    if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT)
                exit(-1);
      // exit(-1);

    /* Create an EventSet */
    EventSet = PAPI_NULL;
    if (PAPI_create_eventset(&EventSet) != PAPI_OK)
        exit(-1);

//---------------------------------------------------------------------
   while(1)
   {

          gettimeofday (&tv1, NULL);
          itime = time(NULL);//从1970年－1-1零点零分到当前系统所偏移的秒数
          pt1 = localtime(&itime);//将从1970－1-1零点零分到当前时间系统所偏移的秒数时间转换为本地时间
          cur_millisec = tv1.tv_usec/1000;



          if(i==0)
	      {                   /**   "%m.nf":输出浮点数，m为宽度，n为小数点右边数位   **/
	      	fprintf(fp,"%d:%d:%d.%d,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%5.2f\n",pt1->tm_hour,pt1->tm_min,pt1->tm_sec,cur_millisec,evt,evt1,evt2,evt3,evt4,evt5,evt6,evt7,evt8,evt9,evt10,evt11,evt12,evt13,evt14,pwr); //put the results into file
              	i++;
              }
              
          if(i==1)
               i++;
          else
	       fprintf(fp,"%d:%d:%d.%d,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%5.2f\n",pt1->tm_hour,pt1->tm_min,pt1->tm_sec,cur_millisec,evt,evt1,evt2,evt3,evt4,evt5,evt6,evt7,evt8,evt9,evt10,evt11,evt12,evt13,evt14,pwr); //put the results into file
		
				
       /*  sleep(1);*/

//---------------------------------------------------

	int k = 0;

	evt = 0;
	evt1 = 0;
	evt2 = 0;
	evt3 = 0;
	evt4 = 0;
	evt5 = 0;
	evt6 = 0;
	evt7 = 0;
	evt8 = 0;
	evt9 = 0;
	evt10 = 0;
	evt11 = 0;
	evt12 = 0;
	evt13 = 0;
	evt14 = 0;



	while(k<5)
	{

	//------1)
		 
    		if (PAPI_add_event(EventSet, PAPI_L3_TCM) != PAPI_OK)
        		exit(-1);
    		if (PAPI_add_event(EventSet, PAPI_L1_LDM) != PAPI_OK)
        		exit(-1);
    		if (PAPI_add_event(EventSet, PAPI_L1_STM ) != PAPI_OK)
        		exit(-1);

    		
   		if (PAPI_start(EventSet) != PAPI_OK)
        		exit(-1);

    		

    		if (PAPI_read(EventSet, values1) != PAPI_OK)
        		exit(-1);

    		usleep(40000);

           	
        	if (PAPI_read(EventSet, values2) != PAPI_OK)
                	exit(-1);
		evt += values2[0]-values1[0];
		evt1 += values2[1]-values1[1];
		evt2 += values2[2]-values1[2];

    		
    		if (PAPI_stop(EventSet, values3) != PAPI_OK)
        		exit(-1);

    		
    		if (PAPI_cleanup_eventset(EventSet) != PAPI_OK)
        		exit(-1);
	//----2)
//usleep(40000);
 		
    		if (PAPI_add_event(EventSet, PAPI_L2_LDM) != PAPI_OK)
        		exit(-1);
    		if (PAPI_add_event(EventSet,PAPI_L2_STM) != PAPI_OK)//PAPI_SP_OPS  
        		exit(-1);
    		if (PAPI_add_event(EventSet, PAPI_PRF_DM ) != PAPI_OK)
        		exit(-1);

    		

   		if (PAPI_start(EventSet) != PAPI_OK)
        		exit(-1);

    		

    		if (PAPI_read(EventSet, values1) != PAPI_OK)
        		exit(-1);

    		usleep(40000);

           	
        	if (PAPI_read(EventSet, values2) != PAPI_OK)
                	exit(-1);
		evt3 += values2[0]-values1[0];
		evt4 += values2[1]-values1[1];
		evt5 += values2[2]-values1[2];

    		
    		if (PAPI_stop(EventSet, values3) != PAPI_OK)
        		exit(-1);

    		
    		if (PAPI_cleanup_eventset(EventSet) != PAPI_OK)
        		exit(-1);

	//-----3)
//usleep(40000);
 		
    		if (PAPI_add_event(EventSet, PAPI_BR_CN) != PAPI_OK)
        		exit(-1);
    		if (PAPI_add_event(EventSet, PAPI_TOT_INS) != PAPI_OK)//PAPI_SP_OPS   
        		exit(-1);
    		if (PAPI_add_event(EventSet, PAPI_LD_INS ) != PAPI_OK)
        		exit(-1);

    		

   		if (PAPI_start(EventSet) != PAPI_OK)
        		exit(-1);

    		

    		if (PAPI_read(EventSet, values1) != PAPI_OK)
        		exit(-1);

    		usleep(40000);

           	
        	if (PAPI_read(EventSet, values2) != PAPI_OK)
                	exit(-1);
		evt6 += values2[0]-values1[0];
		evt7 += values2[1]-values1[1];
		evt8 += values2[2]-values1[2];

    		
    		if (PAPI_stop(EventSet, values3) != PAPI_OK)
        		exit(-1);

    		
    		if (PAPI_cleanup_eventset(EventSet) != PAPI_OK)
        		exit(-1);

	//------4)
//usleep(40000);

 		
    		if (PAPI_add_event(EventSet, PAPI_LST_INS) != PAPI_OK)//PAPI_SP_OPS     
        		exit(-1);
    		if (PAPI_add_event(EventSet, PAPI_BR_INS) != PAPI_OK)
        		exit(-1);
    		if (PAPI_add_event(EventSet, PAPI_TOT_CYC ) != PAPI_OK)
        		exit(-1);

    		

   		if (PAPI_start(EventSet) != PAPI_OK)
        		exit(-1);

    		

    		if (PAPI_read(EventSet, values1) != PAPI_OK)
        		exit(-1);

    		usleep(40000);

           	
        	if (PAPI_read(EventSet, values2) != PAPI_OK)
                	exit(-1);
		evt9 += values2[0]-values1[0];
		evt10 += values2[1]-values1[1];
		evt11 += values2[2]-values1[2];

    		
    		if (PAPI_stop(EventSet, values3) != PAPI_OK)
        		exit(-1);

    		
    		if (PAPI_cleanup_eventset(EventSet) != PAPI_OK)
        		exit(-1);

	//----5)
//usleep(400000);
 		
    		if (PAPI_add_event(EventSet, PAPI_SR_INS) != PAPI_OK)
        		exit(-1);
    	/*	if (PAPI_add_event(EventSet, PAPI_MEM_WCY) != PAPI_OK)
        		exit(-1);*/
    		if (PAPI_add_event(EventSet, PAPI_DP_OPS ) != PAPI_OK)
        		exit(-1);

    		

   		if (PAPI_start(EventSet) != PAPI_OK)
        		exit(-1);

    		

    		if (PAPI_read(EventSet, values1) != PAPI_OK)
        		exit(-1);

    		usleep(40000);

           	
        	if (PAPI_read(EventSet, values2) != PAPI_OK)
                	exit(-1);
		evt12 += values2[0]-values1[0];
		//evt13 += values2[1]-values1[1];
		evt14 += values2[2]-values1[2];

    		
    		if (PAPI_stop(EventSet, values3) != PAPI_OK)
        		exit(-1);

    		
    		if (PAPI_cleanup_eventset(EventSet) != PAPI_OK)
        		exit(-1);
		

		k++;
	}

            i++;
//----------------------------------------------------------------------------
	
	pwr1 = total_J;
	//printf("%5.2f\n",total_J);

	gRapl->TotalEnergy(pkg_J,cores_J,gpu_J,ram_J);
  	// We should have pkg and cores estimates, but might not have gpu and ram
  	// estimates.
  	assert(pkg_J   != kUnsupported_j);    //assert计算表达式，0为假，并输出
  	assert(cores_J != kUnsupported_j);


	

  	// Core and GPU power are a subset of the package power.
  	assert(pkg_J >= cores_J + gpu_J);

  	// Compute "other" (i.e. rest of the package) and "total" only after the
  	// other values have been normalized.
  	other_J = pkg_J - cores_J - gpu_J;

        total_J = pkg_J + ram_J;
  	pwr2 = total_J;

	pwr = (double)(pwr2-pwr1);
	//printf("%5.2f\n",pwr);
	//pwr1 = pwr2;        
//---------------------------------------------------------------------------
        }
     

    fclose(fp);

    if (PAPI_destroy_eventset(&EventSet) != PAPI_OK)
        exit(-1);

    pthread_exit(NULL);// exit the thread
}





















