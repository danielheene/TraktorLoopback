//
//  TraktorLoopback.h
//  TraktorLoopback
//
//  Copyright (C) 2019 Existential Audio Inc.
//
//  

#ifndef TraktorLoopback_h
#define TraktorLoopback_h

#endif /* TraktorLoopback_h */

#include <CoreAudio/AudioServerPlugIn.h>
#include <dispatch/dispatch.h>
#include <mach/mach_time.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/syslog.h>

//==================================================================================================
#pragma mark -
#pragma mark Macros
//==================================================================================================

#if TARGET_RT_BIG_ENDIAN
#define    FourCCToCString(the4CC)    { ((char*)&the4CC)[0], ((char*)&the4CC)[1], ((char*)&the4CC)[2], ((char*)&the4CC)[3], 0 }
#else
#define    FourCCToCString(the4CC)    { ((char*)&the4CC)[3], ((char*)&the4CC)[2], ((char*)&the4CC)[1], ((char*)&the4CC)[0], 0 }
#endif

#if DEBUG

    #define    DebugMsg(inFormat, ...)    syslog(LOG_NOTICE, inFormat, ## __VA_ARGS__)

    #define    FailIf(inCondition, inHandler, inMessage)                                    \
    if(inCondition)                                                                \
    {                                                                            \
        DebugMsg(inMessage);                                                    \
        goto inHandler;                                                            \
    }

    #define    FailWithAction(inCondition, inAction, inHandler, inMessage)                    \
    if(inCondition)                                                                \
    {                                                                            \
        DebugMsg(inMessage);                                                    \
        { inAction; }                                                            \
        goto inHandler;                                                            \
        }

#else

    #define    DebugMsg(inFormat, ...)

    #define    FailIf(inCondition, inHandler, inMessage)                                    \
    if(inCondition)                                                                \
    {                                                                            \
    goto inHandler;                                                            \
    }

    #define    FailWithAction(inCondition, inAction, inHandler, inMessage)                    \
    if(inCondition)                                                                \
    {                                                                            \
    { inAction; }                                                            \
    goto inHandler;                                                            \
    }

#endif


//==================================================================================================
#pragma mark -
#pragma mark TraktorLoopback State
//==================================================================================================

//    The driver has the following
//    qualities:
//    - a box
//    - a device
//        - supports 44100, 48000, 88200, 96000, 176400 and 192000 sample rates
//        - provides a rate scalar of 1.0 via hard coding
//    - a single output stream
//        - supports 16 channels of 32 bit float LPCM samples
//        - writes to ring buffer
//    - a single input stream
//        - supports 16 channels of 32 bit float LPCM samples
//        - reads from ring buffer
//    - controls
//        - master input volume
//        - master output volume
//        - master input mute
//        - master output mute


//    Declare the internal object ID numbers for all the objects this driver implements. Note that
//    because the driver has fixed set of objects that never grows or shrinks. If this were not the
//    case, the driver would need to have a means to dynamically allocate these IDs. It is important
//    to realize that a lot of the structure of this driver is vastly simpler when the IDs are all
//    known a priori. Comments in the code will try to identify some of these simplifications and
//    point out what a more complicated driver will need to do.
enum
{
    kObjectID_PlugIn                    = kAudioObjectPlugInObject,
    kObjectID_Box                        = 2,
    kObjectID_Device                    = 3,
    kObjectID_Stream_Input                = 4,
    kObjectID_Volume_Input_Master        = 5,
    kObjectID_Mute_Input_Master            = 6,
    kObjectID_DataSource_Input_Master    = 7,
    kObjectID_Stream_Output                = 8,
    kObjectID_Volume_Output_Master        = 9,
    kObjectID_Mute_Output_Master        = 10,
    kObjectID_DataSource_Output_Master    = 11
};

//    Declare the stuff that tracks the state of the plug-in, the device and its sub-objects.
//    Note that we use global variables here because this driver only ever has a single device. If
//    multiple devices were supported, this state would need to be encapsulated in one or more structs
//    so that each object's state can be tracked individually.
//    Note also that we share a single mutex across all objects to be thread safe for the same reason.


#define                             kPlugIn_BundleID                    "app.audiosphere.TraktorLoopback"
static pthread_mutex_t              gPlugIn_StateMutex                  = PTHREAD_MUTEX_INITIALIZER;
static UInt32                       gPlugIn_RefCount                    = 0;
static AudioServerPlugInHostRef     gPlugIn_Host                        = NULL;

#define                             kBox_UID                            "TraktorLoopback_UID"
static CFStringRef                  gBox_Name                           = NULL;
static Boolean                      gBox_Acquired                       = true;

#define                             kDevice_UID                         "TraktorLoopback_UID"
#define                             kDevice_ModelUID                    "TraktorLoopback_ModelUID"
static pthread_mutex_t              gDevice_IOMutex                     = PTHREAD_MUTEX_INITIALIZER;
static Float64                      gDevice_SampleRate                  = 48000.0;
static UInt64                       gDevice_IOIsRunning                 = 0;
static const UInt32                 kDevice_RingBufferSize              = 16384;
static Float64                      gDevice_HostTicksPerFrame           = 0.0;
static UInt64                       gDevice_NumberTimeStamps            = 0;
static Float64                      gDevice_AnchorSampleTime            = 0.0;
static UInt64                       gDevice_AnchorHostTime              = 0;

static bool                         gStream_Input_IsActive              = true;
static bool                         gStream_Output_IsActive             = true;

static const Float32                kVolume_MinDB                       = -64.0;
static const Float32                kVolume_MaxDB                       = 0.0;
static Float32                      gVolume_Input_Master_Value          = 1.0;
static Float32                      gVolume_Output_Master_Value         = 1.0;

static bool                         gMute_Input_Master_Value            = false;
static bool                         gMute_Output_Master_Value           = false;

static const UInt32                 kDataSource_NumberItems             = 1;
#define                             kDataSource_ItemNamePattern         "TraktorLoopback"

#define                             DEVICE_NAME                         "TraktorLoopback"
#define                             MANUFACTURER_NAME                   "Audiosphere"

static UInt32                       gDataSource_Input_Master_Value      = 0;
static UInt32                       gDataSource_Output_Master_Value     = 0;

#define                             LATENCY_FRAME_SIZE                  0
#define                             NUMBER_OF_CHANNELS                  8
#define                             BITS_PER_CHANNEL                    32
#define                             BYTES_PER_CHANNEL                   (BITS_PER_CHANNEL / 8)
#define                             BYTES_PER_FRAME                     (NUMBER_OF_CHANNELS * BYTES_PER_CHANNEL)
#define                             RING_BUFFER_FRAME_SIZE               ((32768 + LATENCY_FRAME_SIZE) * NUMBER_OF_CHANNELS)
static Float32*                     ringBuffer;
static UInt64                       ringBufferOffset                    = 0;
//static UInt64                       inIOBufferFrameSize                 = 0;
static UInt64                       remainingRingBufferFrameSize        = 0;

//kAudioHardwarePropertySleepingIsAllowed


//==================================================================================================
#pragma mark -
#pragma mark AudioServerPlugInDriverInterface Implementation
//==================================================================================================

#pragma mark Prototypes

//    Entry points for the COM methods
void*                TraktorLoopback_Create(CFAllocatorRef inAllocator, CFUUIDRef inRequestedTypeUUID);
static HRESULT        TraktorLoopback_QueryInterface(void* inDriver, REFIID inUUID, LPVOID* outInterface);
static ULONG        TraktorLoopback_AddRef(void* inDriver);
static ULONG        TraktorLoopback_Release(void* inDriver);
static OSStatus        TraktorLoopback_Initialize(AudioServerPlugInDriverRef inDriver, AudioServerPlugInHostRef inHost);
static OSStatus        TraktorLoopback_CreateDevice(AudioServerPlugInDriverRef inDriver, CFDictionaryRef inDescription, const AudioServerPlugInClientInfo* inClientInfo, AudioObjectID* outDeviceObjectID);
static OSStatus        TraktorLoopback_DestroyDevice(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID);
static OSStatus        TraktorLoopback_AddDeviceClient(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, const AudioServerPlugInClientInfo* inClientInfo);
static OSStatus        TraktorLoopback_RemoveDeviceClient(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, const AudioServerPlugInClientInfo* inClientInfo);
static OSStatus        TraktorLoopback_PerformDeviceConfigurationChange(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt64 inChangeAction, void* inChangeInfo);
static OSStatus        TraktorLoopback_AbortDeviceConfigurationChange(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt64 inChangeAction, void* inChangeInfo);
static Boolean        TraktorLoopback_HasProperty(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress);
static OSStatus        TraktorLoopback_IsPropertySettable(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, Boolean* outIsSettable);
static OSStatus        TraktorLoopback_GetPropertyDataSize(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32* outDataSize);
static OSStatus        TraktorLoopback_GetPropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, UInt32* outDataSize, void* outData);
static OSStatus        TraktorLoopback_SetPropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData);
static OSStatus        TraktorLoopback_StartIO(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt32 inClientID);
static OSStatus        TraktorLoopback_StopIO(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt32 inClientID);
static OSStatus        TraktorLoopback_GetZeroTimeStamp(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt32 inClientID, Float64* outSampleTime, UInt64* outHostTime, UInt64* outSeed);
static OSStatus        TraktorLoopback_WillDoIOOperation(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt32 inClientID, UInt32 inOperationID, Boolean* outWillDo, Boolean* outWillDoInPlace);
static OSStatus        TraktorLoopback_BeginIOOperation(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt32 inClientID, UInt32 inOperationID, UInt32 inIOBufferFrameSize, const AudioServerPlugInIOCycleInfo* inIOCycleInfo);
static OSStatus        TraktorLoopback_DoIOOperation(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, AudioObjectID inStreamObjectID, UInt32 inClientID, UInt32 inOperationID, UInt32 inIOBufferFrameSize, const AudioServerPlugInIOCycleInfo* inIOCycleInfo, void* ioMainBuffer, void* ioSecondaryBuffer);
static OSStatus        TraktorLoopback_EndIOOperation(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt32 inClientID, UInt32 inOperationID, UInt32 inIOBufferFrameSize, const AudioServerPlugInIOCycleInfo* inIOCycleInfo);

//    Implementation
static Boolean        TraktorLoopback_HasPlugInProperty(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress);
static OSStatus        TraktorLoopback_IsPlugInPropertySettable(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, Boolean* outIsSettable);
static OSStatus        TraktorLoopback_GetPlugInPropertyDataSize(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32* outDataSize);
static OSStatus        TraktorLoopback_GetPlugInPropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, UInt32* outDataSize, void* outData);
static OSStatus        TraktorLoopback_SetPlugInPropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, UInt32* outNumberPropertiesChanged, AudioObjectPropertyAddress outChangedAddresses[2]);

static Boolean        TraktorLoopback_HasBoxProperty(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress);
static OSStatus        TraktorLoopback_IsBoxPropertySettable(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, Boolean* outIsSettable);
static OSStatus        TraktorLoopback_GetBoxPropertyDataSize(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32* outDataSize);
static OSStatus        TraktorLoopback_GetBoxPropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, UInt32* outDataSize, void* outData);
static OSStatus        TraktorLoopback_SetBoxPropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, UInt32* outNumberPropertiesChanged, AudioObjectPropertyAddress outChangedAddresses[2]);

static Boolean        TraktorLoopback_HasDeviceProperty(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress);
static OSStatus        TraktorLoopback_IsDevicePropertySettable(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, Boolean* outIsSettable);
static OSStatus        TraktorLoopback_GetDevicePropertyDataSize(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32* outDataSize);
static OSStatus        TraktorLoopback_GetDevicePropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, UInt32* outDataSize, void* outData);
static OSStatus        TraktorLoopback_SetDevicePropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, UInt32* outNumberPropertiesChanged, AudioObjectPropertyAddress outChangedAddresses[2]);

static Boolean        TraktorLoopback_HasStreamProperty(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress);
static OSStatus        TraktorLoopback_IsStreamPropertySettable(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, Boolean* outIsSettable);
static OSStatus        TraktorLoopback_GetStreamPropertyDataSize(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32* outDataSize);
static OSStatus        TraktorLoopback_GetStreamPropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, UInt32* outDataSize, void* outData);
static OSStatus        TraktorLoopback_SetStreamPropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, UInt32* outNumberPropertiesChanged, AudioObjectPropertyAddress outChangedAddresses[2]);

static Boolean        TraktorLoopback_HasControlProperty(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress);
static OSStatus        TraktorLoopback_IsControlPropertySettable(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, Boolean* outIsSettable);
static OSStatus        TraktorLoopback_GetControlPropertyDataSize(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32* outDataSize);
static OSStatus        TraktorLoopback_GetControlPropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, UInt32* outDataSize, void* outData);
static OSStatus        TraktorLoopback_SetControlPropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, UInt32* outNumberPropertiesChanged, AudioObjectPropertyAddress outChangedAddresses[2]);

#pragma mark The Interface

static AudioServerPlugInDriverInterface    gAudioServerPlugInDriverInterface =
{
    NULL,
    TraktorLoopback_QueryInterface,
    TraktorLoopback_AddRef,
    TraktorLoopback_Release,
    TraktorLoopback_Initialize,
    TraktorLoopback_CreateDevice,
    TraktorLoopback_DestroyDevice,
    TraktorLoopback_AddDeviceClient,
    TraktorLoopback_RemoveDeviceClient,
    TraktorLoopback_PerformDeviceConfigurationChange,
    TraktorLoopback_AbortDeviceConfigurationChange,
    TraktorLoopback_HasProperty,
    TraktorLoopback_IsPropertySettable,
    TraktorLoopback_GetPropertyDataSize,
    TraktorLoopback_GetPropertyData,
    TraktorLoopback_SetPropertyData,
    TraktorLoopback_StartIO,
    TraktorLoopback_StopIO,
    TraktorLoopback_GetZeroTimeStamp,
    TraktorLoopback_WillDoIOOperation,
    TraktorLoopback_BeginIOOperation,
    TraktorLoopback_DoIOOperation,
    TraktorLoopback_EndIOOperation
};
static AudioServerPlugInDriverInterface*    gAudioServerPlugInDriverInterfacePtr    = &gAudioServerPlugInDriverInterface;
static AudioServerPlugInDriverRef            gAudioServerPlugInDriverRef                = &gAudioServerPlugInDriverInterfacePtr;
