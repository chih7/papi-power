#include <assert.h>
#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include "papi.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>

int sampleInterval_msec = 1000;
int sampleCount = 10;

const int MAX_ADD_EVENTS = 6;

// ./papi_avail | grep Yes | awk '{print $1}' | sed 's/$/&,/g'

static int select_preset_events[] = {PAPI_L1_DCM,
                                     PAPI_L1_ICM,
                                     PAPI_L2_DCM,
                                     PAPI_L2_ICM,
                                     PAPI_L1_TCM,
                                     PAPI_L2_TCM,
                                     PAPI_L3_TCM,
                                     PAPI_CA_SNP,
                                     PAPI_CA_SHR,
                                     PAPI_CA_CLN,
                                     PAPI_CA_ITV,
                                     PAPI_L3_LDM,
                                     PAPI_TLB_DM,
                                     PAPI_TLB_IM,
                                     PAPI_L1_LDM,
                                     PAPI_L1_STM,
                                     PAPI_L2_LDM,
                                     PAPI_L2_STM,
                                     PAPI_PRF_DM,
                                     PAPI_MEM_WCY,
                                     PAPI_STL_ICY,
                                     PAPI_FUL_ICY,
                                     PAPI_STL_CCY,
                                     PAPI_FUL_CCY,
                                     PAPI_BR_UCN,
                                     PAPI_BR_CN,
                                     PAPI_BR_TKN,
                                     PAPI_BR_NTK,
                                     PAPI_BR_MSP,
                                     PAPI_BR_PRC,
                                     PAPI_TOT_INS,
                                     PAPI_LD_INS,
                                     PAPI_SR_INS,
                                     PAPI_BR_INS,
                                     PAPI_RES_STL,
                                     PAPI_TOT_CYC,
                                     PAPI_LST_INS,
                                     PAPI_L2_DCA,
                                     PAPI_L3_DCA,
                                     PAPI_L2_DCR,
                                     PAPI_L3_DCR,
                                     PAPI_L2_DCW,
                                     PAPI_L3_DCW,
                                     PAPI_L2_ICH,
                                     PAPI_L2_ICA,
                                     PAPI_L3_ICA,
                                     PAPI_L2_ICR,
                                     PAPI_L3_ICR,
                                     PAPI_L2_TCA,
                                     PAPI_L3_TCA,
                                     PAPI_L2_TCR,
                                     PAPI_L3_TCR,
                                     PAPI_L2_TCW,
                                     PAPI_L3_TCW,
                                     PAPI_SP_OPS,
                                     PAPI_DP_OPS,
                                     PAPI_VEC_SP,
                                     PAPI_VEC_DP,
                                     PAPI_REF_CYC
                                    };

// ./papi_avail | grep Yes | awk '{print $1}' | sed 's/$/&",/g' | sed 's/^/"&/g'
static char *select_preset_events_name[] = {"PAPI_L1_DCM",
                                            "PAPI_L1_ICM",
                                            "PAPI_L2_DCM",
                                            "PAPI_L2_ICM",
                                            "PAPI_L1_TCM",
                                            "PAPI_L2_TCM",
                                            "PAPI_L3_TCM",
                                            "PAPI_CA_SNP",
                                            "PAPI_CA_SHR",
                                            "PAPI_CA_CLN",
                                            "PAPI_CA_ITV",
                                            "PAPI_L3_LDM",
                                            "PAPI_TLB_DM",
                                            "PAPI_TLB_IM",
                                            "PAPI_L1_LDM",
                                            "PAPI_L1_STM",
                                            "PAPI_L2_LDM",
                                            "PAPI_L2_STM",
                                            "PAPI_PRF_DM",
                                            "PAPI_MEM_WCY",
                                            "PAPI_STL_ICY",
                                            "PAPI_FUL_ICY",
                                            "PAPI_STL_CCY",
                                            "PAPI_FUL_CCY",
                                            "PAPI_BR_UCN",
                                            "PAPI_BR_CN",
                                            "PAPI_BR_TKN",
                                            "PAPI_BR_NTK",
                                            "PAPI_BR_MSP",
                                            "PAPI_BR_PRC",
                                            "PAPI_TOT_INS",
                                            "PAPI_LD_INS",
                                            "PAPI_SR_INS",
                                            "PAPI_BR_INS",
                                            "PAPI_RES_STL",
                                            "PAPI_TOT_CYC",
                                            "PAPI_LST_INS",
                                            "PAPI_L2_DCA",
                                            "PAPI_L3_DCA",
                                            "PAPI_L2_DCR",
                                            "PAPI_L3_DCR",
                                            "PAPI_L2_DCW",
                                            "PAPI_L3_DCW",
                                            "PAPI_L2_ICH",
                                            "PAPI_L2_ICA",
                                            "PAPI_L3_ICA",
                                            "PAPI_L2_ICR",
                                            "PAPI_L3_ICR",
                                            "PAPI_L2_TCA",
                                            "PAPI_L3_TCA",
                                            "PAPI_L2_TCR",
                                            "PAPI_L3_TCR",
                                            "PAPI_L2_TCW",
                                            "PAPI_L3_TCW",
                                            "PAPI_SP_OPS",
                                            "PAPI_DP_OPS",
                                            "PAPI_VEC_SP",
                                            "PAPI_VEC_DP",
                                            "PAPI_REF_CYC"
                                           };



static int  EVENTS_NUM = sizeof(select_preset_events)/sizeof(select_preset_events[0]);

// static int EVENTS_NUM = 6;

//char *select_native_events[EVENTS_NUM];

// The value of argv[0] passed to main(). Used in error messages.
static const char* gArgv0;

// A special value that represents an estimate from an unsupported RAPL domain.
static const double kUnsupported_j = -1.0;

FILE *fp;
struct timeval tv;//前后两次采样时间值
struct tm* pt;
time_t itime;

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

// Print to stdout and flush it, so that the output appears immediately even if
// being redirected through |tee| or anything like that.
static void
PrintAndFlush(const char* aFormat, ...)
{
    va_list vargs;
    va_start(vargs, aFormat);
    vfprintf(fp, aFormat, vargs);
    va_end(vargs);

    fflush(fp);
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
    FILE* sysfp = fopen(filename, "r");
    if (!sysfp) {
        return false;
    }
    if (fscanf(sysfp, aScanfString, aOut) != 1) {
        Abort("fscanf() failed");
    }
    fclose(sysfp);

    return true;
}

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
        // need to special a pid?
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
};

class RAPL
{
    // PKG: The entire package.
    Domain* mPkg;
    // PP0
    Domain* mCores;
    // client only PP1
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
};


// The sample interval, measured in seconds.
static double gSampleInterval_sec = double(sampleInterval_msec) / 1000;

// The platform-specific RAPL-reading machinery.
static RAPL* gRapl;

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

void
detect()
{
    int retval, EventSet = PAPI_NULL;
    unsigned int native = 0x0;

    /* Initialize the PAPI library */
    if((PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT ) {
        Abort("PAPI failed to init.\n");
    }

    /* Create an EventSet */
    EventSet = PAPI_NULL;
    if (PAPI_create_eventset(&EventSet) != PAPI_OK) {
        Abort("PAPI failed to create the event set.\n");
    }

    // The RAPL MSRs update every ~1 ms, but the measurement period isn't exactly
    // 1 ms, which means the sample periods are not exact. "Power Measurement
    // Techniques on Standard Compute Nodes: A Quantitative Comparison" by
    // Hackenberg et al. suggests the following.
    //
    //   "RAPL provides energy (and not power) consumption data without
    //   timestamps associated to each counter update. This makes sampling rates
    //   above 20 Samples/s unfeasible if the systematic error should be below
    //   5%... Constantly polling the RAPL registers will both occupy a processor
    //   core and distort the measurement itself."
    //
    // So warn about this case.
    if (sampleInterval_msec < 50) {
        fprintf(stderr,
                "\nWARNING: sample intervals < 50 ms are likely to produce "
                "inaccurate estimates\n\n");
    }

    // Initialize the platform-specific RAPL reading machinery.
    gRapl = new RAPL();

    //print the table head
    PrintAndFlush("timestamp,");
    for (int i=0; i < EVENTS_NUM; i++) {
        PrintAndFlush("%s,",select_preset_events_name[i]);
    }
    PrintAndFlush("pp0-power,pp1-power,pkg-power,ram-power\n");

    // ============================= table body ==================================
    int accu = 0;
    // every row
    while(true) {

        printf("accu: %d\n", accu);

        gettimeofday (&tv, NULL);
        itime = time(NULL);//从1970年－1-1零点零分到当前系统所偏移的秒数
        pt = localtime(&itime);//将从1970－1-1零点零分到当前时间系统所偏移的秒数时间转换为本地时间
        int cur_millisec = tv.tv_usec/1000;

        PrintAndFlush("%d:%d:%d.%d,", pt->tm_hour,pt->tm_min,pt->tm_sec,cur_millisec);

        // time division multiplexing
        int run = 0;
        const int RUN_TIMES = (EVENTS_NUM - 1) / MAX_ADD_EVENTS + 1;//向上取整
        printf("RUN_TIMES: %d\n", RUN_TIMES);
        for(; run < RUN_TIMES; run++) {
            printf("run: %d\n", run);

            if (PAPI_cleanup_eventset(EventSet) != PAPI_OK) {
                Abort("PAPI_stop error \n");
            }

            for(int i = 0; i < MAX_ADD_EVENTS; i++) {
                printf("int: %d\n", i);
                if(PAPI_query_event(select_preset_events[i + run * MAX_ADD_EVENTS]) == PAPI_OK) {
                    retval = PAPI_add_event(EventSet, select_preset_events[i + run * MAX_ADD_EVENTS]);
                    if(retval != PAPI_OK) {
                        Abort("PAPI_add_event %s error : %d! \n",
                              select_preset_events_name[i + run * MAX_ADD_EVENTS], retval);
                    }
                } else {
                    Abort("PAPI_query_event %s error! \n",
                          select_preset_events_name[i + run * MAX_ADD_EVENTS]);
                }
            }

            if (PAPI_reset(EventSet) != PAPI_OK) {
                Abort("PAPI_reset error! \n");
            }

            if (PAPI_start(EventSet) != PAPI_OK) {
                Abort("PAPI_start error! \n");
            }

            // impossible exceed 128
            long_long values[128] = {0};
            /* Read counters */
            if (PAPI_read(EventSet, values) != PAPI_OK) {
                Abort("PAPI_read error! \n");
            }

            for(int i = 0; i < MAX_ADD_EVENTS; i++) {
                // is values need to * RUN_TIMES
                PrintAndFlush("%lld,",values[i]);
            }

            usleep(sampleInterval_msec * 1000 / RUN_TIMES);

            if (PAPI_stop(EventSet, values) != PAPI_OK) {
                Abort("PAPI_stop error \n");
            }
            if (PAPI_cleanup_eventset(EventSet) != PAPI_OK) {
                Abort("PAPI_stop error \n");
            }

        } // end multiplexing

        double pkg_J, cores_J, gpu_J, ram_J;
        gRapl->EnergyEstimates(pkg_J, cores_J, gpu_J, ram_J);

        // We should have pkg and cores estimates, but might not have gpu and ram
        // estimates.
        assert(pkg_J   != kUnsupported_j);
        assert(cores_J != kUnsupported_j);

        // This needs to be big enough to print watt values to two decimal places. 16
        // should be plenty.
        static const size_t kNumStrLen = 16;

        static char pkgStr[kNumStrLen], coresStr[kNumStrLen], gpuStr[kNumStrLen],
               ramStr[kNumStrLen];
        NormalizeAndPrintAsWatts(pkgStr,   pkg_J);
        NormalizeAndPrintAsWatts(coresStr, cores_J);
        NormalizeAndPrintAsWatts(gpuStr,   gpu_J);
        NormalizeAndPrintAsWatts(ramStr,   ram_J);

        char otherStr[kNumStrLen];
        double other_J = pkg_J - cores_J - gpu_J;
        NormalizeAndPrintAsWatts(otherStr, other_J);

        char totalStr[kNumStrLen];
        double total_J = pkg_J + ram_J;
        NormalizeAndPrintAsWatts(totalStr, total_J);
        PrintAndFlush("%s,%s,%s,%s\n",coresStr,gpuStr,pkgStr,ramStr);

        if(accu >= sampleCount) {
            if (PAPI_destroy_eventset(&EventSet) != PAPI_OK) {
                Abort("PAPI_stop error \n");
            }

            break;
        }
        accu++;
    }
}


// void
// detect_multiplex()
// {
//     int retval, EventSet = PAPI_NULL;
//     unsigned int native = 0x0;
//
//     printf("%d\n", PAPI_num_counters());
//
//     /* Initialize the PAPI library */
//     if((PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT ) {
//         Abort("PAPI failed to init.\n");
//     }
//
//     /* Enable and initialize multiplex support */
//     if (PAPI_multiplex_init() != PAPI_OK)
//         Abort("PAPI_multiplex_init failed.\n");
//
//     /* Create an EventSet */
//     EventSet = PAPI_NULL;
//     if (PAPI_create_eventset(&EventSet) != PAPI_OK) {
//         Abort("PAPI failed to create the event set.\n");
//     }
//     if(PAPI_assign_eventset_component(EventSet, 0) != PAPI_OK) {
//         Abort("PAPI_assign_eventset_component failed\n");
//     }
//     /* Convert the ''EventSet'' to a multiplexed event set */
//     if (PAPI_set_multiplex(EventSet) != PAPI_OK)
//         Abort("PAPI_set_multiplex failed.\n");
//
//     for(int i = 0; i < EVENTS_NUM; i++) {
//         if(PAPI_query_event(select_preset_events[i]) == PAPI_OK) {
//             if(PAPI_add_event(EventSet, select_preset_events[i]) != PAPI_OK) {
//                 Abort("PAPI_add_event %s error! \n", select_preset_events_name[i]);
//             }
//         } else {
//             Abort("PAPI_query_event %s error! \n", select_preset_events_name[i]);
//         }
//     }
//
//     retval = PAPI_get_multiplex(EventSet);
//     if (retval > 0) printf("This event set is ready for multiplexing\n.");
//     if (retval == 0) printf("This event set is not enabled for multiplexing\n.");
//     if (retval < 0)
//         Abort("PAPI_get_multiplex error.\n");
//
//     if (PAPI_start(EventSet) != PAPI_OK) {
//         Abort("PAPI_start error! \n");
//     }
//
//     // The RAPL MSRs update every ~1 ms, but the measurement period isn't exactly
//     // 1 ms, which means the sample periods are not exact. "Power Measurement
//     // Techniques on Standard Compute Nodes: A Quantitative Comparison" by
//     // Hackenberg et al. suggests the following.
//     //
//     //   "RAPL provides energy (and not power) consumption data without
//     //   timestamps associated to each counter update. This makes sampling rates
//     //   above 20 Samples/s unfeasible if the systematic error should be below
//     //   5%... Constantly polling the RAPL registers will both occupy a processor
//     //   core and distort the measurement itself."
//     //
//     // So warn about this case.
//     if (sampleInterval_msec < 50) {
//         fprintf(stderr,
//                 "\nWARNING: sample intervals < 50 ms are likely to produce "
//                 "inaccurate estimates\n\n");
//     }
//
//     // Initialize the platform-specific RAPL reading machinery.
//     gRapl = new RAPL();
//
//     PrintAndFlush("timestamp,");
//     for (int i=0; i < EVENTS_NUM; i++) {
//         PrintAndFlush("%s,",select_preset_events_name[i]);
//     }
//     PrintAndFlush("pp0-power,pp1-power,pkg-power,ram-power\n");
//
//     int accu = 0;
//     while(true) {
//
//         if (PAPI_reset(EventSet) != PAPI_OK) {
//             Abort("PAPI_reset error! \n");
//         }
//
//         gettimeofday (&tv, NULL);
//         itime = time(NULL);//从1970年－1-1零点零分到当前系统所偏移的秒数
//         pt = localtime(&itime);//将从1970－1-1零点零分到当前时间系统所偏移的秒数时间转换为本地时间
//         int cur_millisec = tv.tv_usec/1000;
//
//         long_long values[128] = {0};
//         /* Read counters */
//         if (PAPI_read(EventSet, values) != PAPI_OK) {
//             Abort("PAPI_read error! \n");
//         }
//
//         double pkg_J, cores_J, gpu_J, ram_J;
//         gRapl->EnergyEstimates(pkg_J, cores_J, gpu_J, ram_J);
//
//         // We should have pkg and cores estimates, but might not have gpu and ram
//         // estimates.
//         assert(pkg_J   != kUnsupported_j);
//         assert(cores_J != kUnsupported_j);
//
//         // This needs to be big enough to print watt values to two decimal places. 16
//         // should be plenty.
//         static const size_t kNumStrLen = 16;
//
//         static char pkgStr[kNumStrLen], coresStr[kNumStrLen], gpuStr[kNumStrLen],
//                ramStr[kNumStrLen];
//         NormalizeAndPrintAsWatts(pkgStr,   pkg_J);
//         NormalizeAndPrintAsWatts(coresStr, cores_J);
//         NormalizeAndPrintAsWatts(gpuStr,   gpu_J);
//         NormalizeAndPrintAsWatts(ramStr,   ram_J);
//
//         char otherStr[kNumStrLen];
//         double other_J = pkg_J - cores_J - gpu_J;
//         NormalizeAndPrintAsWatts(otherStr, other_J);
//
//         char totalStr[kNumStrLen];
//         double total_J = pkg_J + ram_J;
//         NormalizeAndPrintAsWatts(totalStr, total_J);
//
//         //fix the first power records all are 0, drop the first record
//         if (accu > 0) {
//             PrintAndFlush("%d:%d:%d.%d,", pt->tm_hour,pt->tm_min,pt->tm_sec,cur_millisec);
//             for(int i = 0; i < EVENTS_NUM; i++) {
//                 PrintAndFlush("%lld,",values[i]);
//             }
//             PrintAndFlush("%s,%s,%s,%s\n",coresStr,gpuStr,pkgStr,ramStr);
//         }
//
//         usleep(sampleInterval_msec * 1000);
//
//         if(accu >= sampleCount) {
//
//             if (PAPI_stop(EventSet, values) != PAPI_OK) {
//                 Abort("PAPI_stop error \n");
//             }
//             if (PAPI_cleanup_eventset(EventSet) != PAPI_OK) {
//                 Abort("PAPI_stop error \n");
//             }
//             if (PAPI_destroy_eventset(&EventSet) != PAPI_OK) {
//                 Abort("PAPI_stop error \n");
//             }
//             break;
//         }
//         accu++;
//     }
// }

int
main()
{
    /*
     *   To free pagecache:
     *      echo 1 > /proc/sys/vm/drop_caches
     *   To free reclaimable slab objects (includes dentries and inodes):
     *       echo 2 > /proc/sys/vm/drop_caches
     *   To free slab objects and pagecache:
     *       echo 3 > /proc/sys/vm/drop_caches
     */
    char string[] ="sync && echo 3 > /proc/sys/vm/drop_caches && sleep 2 \
             && echo 1 > /proc/sys/vm/drop_caches \
             && echo 2 > /proc/sys/vm/drop_caches";
    system(string); //清除系统缓存

    char filename[256];
    itime = time(NULL);
    pt = localtime(&itime);
    sprintf(filename, "util-power-%d-%d.csv", pt->tm_hour, pt->tm_min);
    if((fp = fopen(filename, "ab")) == NULL)
    {
        perror("file open failed!");
        exit(EXIT_FAILURE);
    }

    detect();

    fclose(fp);
    printf("finshed");
    return 0;
}













