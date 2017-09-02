////////////////////////////////////////////////////////////////////////////////
//
//  Kapsy.h - KAPSY'S TOOLBOX
//

/*
   No warranty is offered or implied; use this code at your own risk.

   This is a single header file with a bunch of useful utilities for getting
   stuff done in C/C++.

   Most functions are taken from Variations, which is based largely on
   Handmade Hero, with slight modifications.
   */

#if !defined(KAPSY_H)

#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <float.h>

#include <math.h>

////////////////////////////////////////////////////////////////////////////////
//
//  COMPILERS
//

#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif

#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else
// TODO: (Kapsy) More compilers!
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif

#if COMPILER_MSVC
#include <intrin.h>
#elif COMPILER_LLVM
#include <xmmintrin.h>
#include <emmintrin.h>
#include <pthread.h>
#endif


////////////////////////////////////////////////////////////////////////////////
//
//  TYPEDEFS/DEFINES
//

typedef intptr_t intptr;
typedef uintptr_t uintptr;
typedef size_t memory_index;

#if 0
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef int32_t b32;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#else

typedef char s8;
typedef short s16;
typedef int s32;
typedef long s64;
typedef int b32;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;

#endif

typedef float r32;
typedef double r64;

#define Real32Maximum FLT_MAX

typedef size_t size;
typedef intptr_t intptr;
typedef uintptr_t uintptr;

#define internal static
#define local_persist static
#define global_variable static

#define elif else if
#define Pi32 3.14159265359f
#define Tau32 6.28318530717959f

// NOTE: (Kapsy) Platform level types

#if VARIATIONS_SLOW
// TODO: (Kapsy) abort() is part of stdlib - see if we can create an alternative
// and remove this dependency.
#define Assert(Expression) if(!(Expression)) {abort();}
#else
#define Assert(Expression)
#endif

#define AssertNAN(Number) Assert(!(Number != Number));

#define BitsPerByte 8
#define SizeOfFloat sizeof(float)

#define NSPerUS (1000.0)
#define NSPerMS (1000.0 * NSPerUS)
#define NSPerS  (1000.0 * NSPerMS)

#define USPerMS (1000.0)
#define USPerS  (1000.0 * USPerMS)

#define USFromNS(Value) ((Value) / NSPerUS)
#define USFromMS(Value) ((Value) * USPerMS)
#define USFromS(Value) ((Value) * USPerS)

#define NSFromMS(Value) (Value * NSPerMS)
#define NSFromS(Value) (Value * NSPerS)

#define SFromNS(Value) ((Value) / NSPerS)

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define InvalidCodePath Assert(!"InvalidCodePath");
#define InvalidDefaultCase default: {InvalidCodePath;} break

// TODO: (Kapsy) Swap, Min, Max
// TODO: (Kapsy) Move these to variations_platform_utils.h
#define Align4(Value) ((Value + 3) & ~3)
#define Align8(Value) ((Value + 7) & ~7)
#define Align16(Value) ((Value + 15) & ~15)

inline u32
SafeTruncateUint64(u64 Value)
{
    // TODO: (Kapsy) Defines for maximum values.
    Assert(Value <= 0xFFFFFFFF);
    u32 Result = (u32)Value;
    return(Result);
}

////////////////////////////////////////////////////////////////////////////////
//
//  THREADING
//

typedef struct thread_context
{
    int Placeholder;
} thread_context;

#if COMPILER_MSVC
// TODO: (Kapsy) Upgrade to match LLVM once we start on Win32!
#define CompletePreviousWritesBeforeFutureWrites _WriteBarrier();
#define CompletePreviousReadsBeforeFuturesReads _ReadBarrier();
inline b32 AtomicCompareExchangeUInt32(u32 volatile *TheValue, u32 OldValue, u32 NewValue)
{
    b32 Result = false;
    u32 OriginalValue = _InterlockedCompareExchange((long *)TheValue, NewValue, OldValue);

    if(OriginalValue == OldValue)
    {
        Result = true;
    }

    return(Result);
}

#elif COMPILER_LLVM
// TODO(casey): Need GCC/LLVM equivalents!
//

// TODO: (Kapsy) Implement the ASM version at github.com/itfrombit/osx_handmade
// See: http://clang.llvm.org/docs/LanguageExtensions.html#builtin-functions
#define __rdtsc __builtin_readcyclecounter
#define rdtsc __rdtsc


#if 0
inline u32 GetCurrentThreadID()
{
    u64 ThreadID = 0;
    pthread_threadid_np(0, &ThreadID);
    return((u32)ThreadID);
}
#else
inline u32 GetCurrentThreadID(void)
{
    u32 ThreadID;
#if defined(__APPLE__) && defined(__x86_64__)
    asm("mov %%gs:0x00,%0" : "=r"(ThreadID));
#elif defined(__x86_64__)
    asm("mov %%gs:0x08,%0" : "=r"(ThreadID));
#elif defined(__x86_64__)
    asm("mov %%fs:0x10,%0" : "=r"(ThreadID));
#else
#error Unsupported architecture
#endif
    return(ThreadID);
}
#endif

// TODO: (Kapsy) Does LLVM have specific barrier calls?
#define CompletePreviousReadsBeforeFuturesReads asm volatile("" ::: "memory")
#define CompletePreviousWritesBeforeFutureWrites asm volatile("" ::: "memory")

#if 1

// NOTE: (Kapsy) HMH LLVM atomic operations:
//
// TODO: (Kapsy) From the GCC docs (might be different for LLVM):
// These functions are implemented in terms of the ‘__atomic’ builtins (see __atomic Builtins). They should not be used for new code which should use the ‘__atomic’ builtins instead.

// bool __sync_bool_compare_and_swap (type *ptr, type oldval, type newval, ...)
// type __sync_val_compare_and_swap (type *ptr, type oldval, type newval, ...)
// These built-in functions perform an atomic compare and swap. That is, if the current value of *ptr is oldval, then write newval into *ptr.
// The “bool” version returns true if the comparison is successful and newval is written. The “val” version returns the contents of *ptr before the operation.
// __sync_synchronize (...)
// TODO: (Kapsy) Need a reliable way of testing atomic functions.
// TODO: (Kapsy) Do we need to manually add barriers?
inline u32
AtomicCompareExchangeU32_U32(u32 volatile *TheValue, u32 OldValue, u32 NewValue)
{
    u32 Result = __sync_val_compare_and_swap(TheValue, OldValue, NewValue);
    return(Result);
}

inline b32
AtomicCompareExchangeU32_B32(u32 volatile *TheValue, u32 OldValue, u32 NewValue)
{
    b32 Result = __sync_bool_compare_and_swap(TheValue, OldValue, NewValue);
    return(Result);
}

inline u64
AtomicExchangeU64_U64(u64 volatile *TheValue, u64 NewValue)
{
    u64 Result = __sync_lock_test_and_set(TheValue, NewValue);
    return(Result);
}

inline u64
AtomicAddU64_U64(u64 volatile *TheValue, u64 Addend)
{
    // NOTE: (Kapsy) Returns the original value, prior to adding.
    u64 Result = __sync_fetch_and_add(TheValue, Addend);
    return(Result);
}


#else

// NOTE: (Kapsy) OS X atomic operations:

#include <libkern/OSAtomic.h>
#define CompletePreviousWritesBeforeFutureWrites OSMemoryBarrier();

inline b32
AtomicCompareExchangeU32_B32(u32 *TheValue, u32 OldValue, u32 NewValue)
{
//  @abstract Compare and swap for <code>int</code> values.
//    @discussion
//    This function compares the value in <code>__oldValue</code> to the value
//    in the memory location referenced by <code>__theValue</code>.  If the values
//    match, this function stores the value from <code>__newValue</code> into
//    that memory location atomically.
//    This function is equivalent to {@link OSAtomicCompareAndSwap32}.
//    @result Returns TRUE on a match, FALSE otherwise.
// bool OSAtomicCompareAndSwapInt( int __oldValue, int __newValue, volatile int *__theValue );

    b32 Result = (b32)OSAtomicCompareAndSwapIntBarrier((int)OldValue, (int)NewValue, (int volatile *)TheValue);
    return(Result);
}

#endif

#endif

////////////////////////////////////////////////////////////////////////////////
//
//  SIMPLE CYCLE COUNTER
//

global_variable long long kapsy_cycles_start;

internal void
kapsy_cycles_init()
{
    kapsy_cycles_start = rdtsc();
}

internal double
kapsy_m_cycles_reset()
{
    long long end_count = rdtsc();
    double mc = (double)(end_count - kapsy_cycles_start) / (1000.f * 1000.f);
    kapsy_cycles_start = end_count;
    return(mc);
}

internal void
kapsy_m_cycles_reset_and_print()
{

    long long end_count = rdtsc();
    double mc = (double)(end_count - kapsy_cycles_start) / (1000.f * 1000.f);
    kapsy_cycles_start = end_count;
    printf("%f megacycles\n", mc);
}

////////////////////////////////////////////////////////////////////////////////
//
//  LINKED LIST
//

#define DLIST_INSERT_BEFORE(Element, Base) \
    Element->Prev = Base->Prev; \
    Element->Next = Base; \
    Base->Prev->Next = Element; \
    Base->Prev = Element \

#define DLIST_INSERT_AFTER(Element, Base) \
    Element->Prev = Base; \
    Element->Next = Base->Next; \
    Base->Next->Prev = Element; \
    Base->Next = Element \

#define DLIST_REMOVE(Element) \
    Element->Next->Prev = Element->Prev; \
    Element->Prev->Next = Element->Next; \

#define DLIST_INIT(Sentinal) \
    (Sentinal)->Next = (Sentinal); \
    (Sentinal)->Prev = (Sentinal); \

#define FREELIST_ALLOC(Result, NextFree, AllocationCode) \
    Result = (NextFree); \
    if((NextFree)->Prev) \
    { \
        (NextFree) = (NextFree)->Prev; \
    } \
    else \
    { \
        (NextFree) = (AllocationCode); \
    } \

#define FREELIST_ALLOC_CLEAR(Result, NextFree, AllocationCode) \
    Result = (NextFree); \
    if((NextFree)->Prev) \
    { \
        (NextFree) = (NextFree)->Prev; \
    } \
    else \
    { \
        (NextFree) = (AllocationCode); \
        ZeroStruct(*NextFree); \
    } \

////////////////////////////////////////////////////////////////////////////////
//
//  DEBUG MACROS
//
//  TODO: (Kapsy) Not sure if I want to use these!

#if VARIATIONS_INTERNAL
    #define DEBUG_INTERNAL(Code) \
        Code
#else
    #define DEBUG_INTERNAL(Code)
#endif

#if VARIATIONS_INTERNAL
    #define BEGIN_DEBUG_IF(Var) \
        if((GlobalDebugVars + Var)->Value.B32) \
        {
#else
    #define BEGIN_DEBUG_IF(Var)
#endif

#if VARIATIONS_INTERNAL
    #define DEBUG_ELSE \
        } \
        else \
        {
#else
    #define DEBUG_ELSE
#endif

#if VARIATIONS_INTERNAL
    #define END_DEBUG_IF \
        }
#else
    #define END_DEBUG_IF
#endif


#if VARIATIONS_INTERNAL
    #define DEBUG_R32(Target, Var) \
        (Target) = (GlobalDebugVars + Var)->Value.R32
#else
    #define DEBUG_R32(Target, Var)
#endif

#if VARIATIONS_INTERNAL
    #define DEBUG_U32(Target, Var) \
        (Target) = (GlobalDebugVars + Var)->Value.U32
#else
    #define DEBUG_U32(Target, Var)
#endif


///////////////////////////////////////////////////////////////////////////////
// Program Args ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
kapsy_PrintArgs(int argc, char **argv)
{
    printf("argc: %d argv: ", argc);
    for(u32 ArgIndex = 0;
            ArgIndex < argc;
            ++ArgIndex)
    {
        char *Arg = *(argv + ArgIndex);
        printf("%s ", Arg);
    }
    printf("\n");
}


////////////////////////////////////////////////////////////////////////////////
//
//  Strings
//

// TODO: (Kapsy) Naming:
// Thinking of something like:
// K_StringLength
// kapsy_StringLength
// kapsy_strlen
// kap_strlen
// Kap_StringLength

#if 0

inline int
StringLength(char *String)
{
    int Count = 0;
    while(*String++)
    {
        ++Count;
    }
    return(Count);
}

inline void
CatStrings(size SourceACount, char *SourceA,
        size SourceBCount, char *SourceB,
        size DestCount, char *Dest)
{
    for(int Index = 0;
    Index < SourceACount;
    ++Index)
    {
        *Dest++ = *SourceA++;
    }

    for(int Index = 0;
    Index < SourceBCount;
    ++Index)
    {
        *Dest++ = *SourceB++;
    }

    *Dest = 0;
}
#endif
// TODO: (Kapsy) A string where the length is a word at the start of the pointer?
struct string
{
    char *Chars;
    int Length;
};

inline int
StringLength(char *Chars)
{
    int Result = 0;
    while(*Chars++ != '\0')
    {
        ++Result;
    }

    return(Result);
}

inline string
CreateString(char *Chars)
{
    string Result = {};
    Result.Chars = Chars;
    Result.Length = StringLength(Chars);

    return(Result);
}

inline int
StringCopy(char *A, char *B)
{
    int Count = 0;
    while(*A)
    {
        *B++ = *A++;
        ++Count;
    }

    return(Count);
}

inline int
StringCopy(string A, char *B)
{
    int Count = 0;
    while(Count < A.Length)
    {
        *B++ = *(A.Chars + Count);
        ++Count;
    }

    return(Count);
}

static bool
kapsy_StringComp(char *A, char *B)
{
    bool Result = true;
    while(*A)
    {
        if(*A++ != *B++)
        {
            Result = false;
            break;
        }
    }

    return(Result);
}

static bool
StringsAreEqual(string A, string B)
{
    bool Result = false;
    if(A.Length == B.Length)
    {
        Result = true;

        for(int Index = 0;
                Index < A.Length;
                ++Index)
        {
            if(A.Chars[Index] != B.Chars[Index])
            {
                Result = false;
                break;
            }
        }
    }
    return(Result);
}


// TODO: (Kapsy) This is really janky! Just want to use one or the other string type.
// Although, I have a feeling that I can't use the pointer + length version without metaprogramming.
inline bool
StringsAreEqual(string A, char *B)
{
    bool Result = true;

    for(int Index = 0;
            Index < A.Length;
            ++Index)
    {

        if(*B == 0)
        {
            Result = false;
            break;
        }

        if(A.Chars[Index] != *B++)
        {
            Result = false;
            break;
        }
    }

    if(*B != 0)
    {
        Result = false;
    }

    return(Result);
}

static bool
StringStartsWith(string A, string Start)
{
    bool Result = false;

    if(A.Length >= Start.Length)
    {
        A.Length = Start.Length;

        Result = StringsAreEqual(A, Start);
    }

    return(Result);
}

static bool
StringEndsWith(string A, string End)
{
    bool Result = false;

    if(End.Length <= A.Length)
    {
        string AEnd = { A.Chars + (A.Length - End.Length), End.Length };
        Result = StringsAreEqual(AEnd, End);
    }

    return(Result);
}

inline bool
StringContainsChar(string A, char B)
{
    bool Result = false;

    char *AAt = A.Chars;
    int ARemaining = A.Length;

    while(ARemaining--)
    {
        if(*AAt == B)
        {
            Result = true;
            break;
        }
    }

    return(Result);
}

// TODO: (Kapsy) Let user malloc!
#if 0
static char *
ConcatStrings(char *A, string B)
{
    int ResultLength = StringLength(A) + B.Length + 1;

    char *Result = (char *)malloc(ResultLength);
    char *ResultAt = Result;

    char *AAt = A;
    char *BAt = B.Chars;

    int BPos = 0;

    while(*AAt)
    {
        *ResultAt++ = *AAt++;
    }
    while(BPos++ < B.Length)
    {
        *ResultAt++ = *BAt++;
    }
    *ResultAt = '\0';
    return(Result);
}
#endif

internal string
ConcatStrings(char *Buffer, u32 *Written, string A, string B)
{
    u32 Length = A.Length + B.Length;
    string Result = { Buffer, Length };
    char *ResultAt = Result.Chars;

    u32 ARemaining = A.Length;
    char *AAt = A.Chars;
    while(ARemaining--)
    {
        *ResultAt++ = *AAt++;
    }

    u32 BRemaining = B.Length;
    char *BAt = B.Chars;
    while(BRemaining--)
    {
        *ResultAt++ = *BAt++;
    }

    *ResultAt = '\0';
    *Written = (u32)(ResultAt - Buffer);

    return(Result);
}

internal string
ConcatStrings(string A, string B)
{
    u32 Length = A.Length + B.Length;
    char *Chars = (char *)malloc(Length + 1);

    string Result = { Chars, Length };
    char *ResultAt = Result.Chars;

    u32 ARemaining = A.Length;
    char *AAt = A.Chars;
    while(ARemaining--)
    {
        *ResultAt++ = *AAt++;
    }

    u32 BRemaining = B.Length;
    char *BAt = B.Chars;
    while(BRemaining--)
    {
        *ResultAt++ = *BAt++;
    }

    *ResultAt = '\0';
    return(Result);
}

internal string
RemoveLastPathElement(string Path)
{
    string Result = Path;

    char *At = Path.Chars + Path.Length - 1;
    while(At != Path.Chars)
    {
        if(*At == '/')
        {
            Result.Length = At - Result.Chars + 1;
            *(At + 1) = '\0';
            break;
        }
        --At;
    }

    return(Result);
}


////////////////////////////////////////////////////////////////////////////////
//
//  File Operations
//

// TODO: (Kapsy) A function that returns the file size so the implementer can malloc.

static char *
ReadEntireFile(char *Filename)
{
    char *FileMemory = 0;
    FILE *File = fopen(Filename, "r");

    if(File) {

        fseek(File, 0, SEEK_END);
        size_t Size = ftell(File);
        fseek(File, 0, SEEK_SET);

        FileMemory = (char *)malloc(Size);
        size_t Read = fread(FileMemory, sizeof(char), Size + 1, File);
        FileMemory[Size] = 0;
    }

    return(FileMemory);
}

#if 0
struct file_group
{
    u32 Count;
    u32 Current;
    b32 HasErrors;
};

struct file_handle
{
    // TODO: (Kapsy) Should be an s32, for OSX at least.
    u32 Handle;
    b32 HasErrors;
};

// NOTE: (Kapsy) As the name implies, these are for debugging only!
typedef struct debug_read_file_result
{
    u32 ContentsSize;
    void *Contents;

} debug_read_file_result;

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void *Memory, u32 Size)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(char *Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) b32 name(char *Filename, void *Memory, u32 Size)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#define DEBUG_PLATFORM_CREATE_TIMER(name) b32 name(char *File, char *Function, u32 Line, u32 Index)
typedef DEBUG_PLATFORM_CREATE_TIMER(debug_platform_create_debug_timer);




struct platform_work_queue;
#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(platform_work_queue *Queue, void *Data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);

typedef void platform_add_entry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data);
typedef void platform_complete_all_work(platform_work_queue *Queue);

#define PLATFORM_FILES_FOR_EXTENSION(name) file_group *name(char *Extension)
typedef PLATFORM_FILES_FOR_EXTENSION(platform_files_for_extension);

#define PLATFORM_OPEN_NEXT_FILE_IN_GROUP(name) file_handle name(file_group *FileGroup)
typedef PLATFORM_OPEN_NEXT_FILE_IN_GROUP(platform_open_next_file_in_group);

#define PLATFORM_READ_FROM_FILE(name) void name(file_handle *Handle, void *Dest, u32 Size, u32 Offset)
typedef PLATFORM_READ_FROM_FILE(platform_read_from_file);

#define PLATFORM_CLOSE_FILE_GROUP(name) void name(file_group *FileGroup)
typedef PLATFORM_CLOSE_FILE_GROUP(platform_close_file_group);

struct platform_api
{
    platform_add_entry *AddEntry;
    platform_complete_all_work *CompleteAllWork;

    debug_platform_free_file_memory *DEBUGFreeFileMemory;
    debug_platform_read_entire_file *DEBUGReadEntireFile;
    debug_platform_write_entire_file *DEBUGWriteEntireFile;

    platform_files_for_extension *FilesForExtension;
    platform_open_next_file_in_group *OpenNextFileInGroup;
    platform_read_from_file *ReadFromFile;
    platform_close_file_group *CloseFileGroup;
};
#endif

////////////////////////////////////////////////////////////////////////////////
//
//  INTRINSICS
//
//

// TODO: (Kapsy) Convert all of these to platform efficient versions.

inline u32
PointerToU32(void *Pointer)
{
    u32 Result = (u32)((u64)Pointer);
    return(Result);
}

#define ZeroArray(Array) ZeroSize(sizeof(Array), Array);
#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
inline void
ZeroSize(memory_index Size, void *Ptr)
{
    // TODO: (Kapsy) Check this guy for performance.
    u8 *Byte = (u8 *)Ptr;
    while(Size--)
    {
        *Byte++ = 0;
    }
}

inline void
MemCopy(void *From, void *To, u32 Size)
{
    u8 *FromBytes = (u8 *)From;
    u8 *ToBytes = (u8 *)To;

    for(int Index = 0;
            Index < Size;
            ++Index)
    {
        *ToBytes++ = *FromBytes++;
    }
}

// TODO: (Kapsy) Switch to this!
inline void
Copy(void *From, void *To, u32 Size)
{
    u8 *FromBytes = (u8 *)From;
    u8 *ToBytes = (u8 *)To;

    while(Size--) { *ToBytes++ = *FromBytes++; }
}

inline s32
SignOf(s32 Value)
{
    s32 Result = (Value >= 0) ? 1 : -1;
    return(Result);
}

inline r32
SquareRoot(r32 Real32)
{
    r32 Result = sqrtf(Real32);
    return(Result);
}

inline r32
Pow64(r32 Base, r32 Exponent)
{
    r32 Result = (r32)pow((r64)Base, (r64)Exponent);
    return(Result);
}

inline r32
AbsoluteValue(r32 Real32)
{
    r32 Result = fabs(Real32);
    return(Result);
}

inline u32
RotateLeft(u32 Value, s32 Amount)
{
#if COMPILER_MSVC
    u32 Result = _rotl(Value, Amount);
#else
    // TODO: (Kapsy) Other compiler support!
    // u32 Result = rol32(Value, Amount);
    Amount &= 31;
    u32 Result = ((Value << Amount) | (Value >> (32 - Amount)));
#endif
    return(Result);
}

inline u32
RotateRight(u32 Value, s32 Amount)
{
#if COMPILER_MSVC
    u32 Result = _rotr(Value, Amount);
#else
    // TODO: (Kapsy) Other compiler support!
    // u32 Result = ror32(Value, Amount);
    Amount &= 31;
    u32 Result = ((Value >> Amount) | (Value << (32 - Amount)));
#endif
    return(Result);
}

inline r32
RoundReal32(r32 Real32)
{
    r32 Result = roundf(Real32);
    return(Result);
}

inline r32
RoundReal32DownLeaning(r32 Real32)
{
    Real32-=0.000001f;
    r32 Result = roundf(Real32);
    return(Result);
}

inline s32
RoundReal32ToInt32(r32 Real32)
{
    s32 Result = (s32)roundf(Real32);
    return(Result);
}

inline u32
RoundReal32ToUInt32(r32 Real32)
{
    u32 Result = (u32)roundf(Real32);
    return(Result);
}

inline s32
FloorReal32ToInt32(r32 Real32)
{
    // TODO: (Kapsy) Should be calling the builtin floorf - although I don't know how to check!
    s32 Result = (s32)__builtin_floorf(Real32);
    return(Result);
}

inline r32
FloorReal32(r32 Real32)
{
    // TODO: (Kapsy) Should be calling the builtin floorf - although I don't know how to check!
    r32 Result = __builtin_floorf(Real32);
    return(Result);
}

inline s32
CeilReal32ToInt32(r32 Real32)
{
    s32 Result = (s32)__builtin_ceilf(Real32);
    return(Result);
}

inline u32
CeilReal32ToUInt32(r32 Real32)
{
    u32 Result = (u32)__builtin_ceilf(Real32);
    return(Result);
}

inline s32
TruncateReal32ToInt32(r32 Real32)
{
    s32 Result = (s32)Real32;
    return(Result);
}

inline r32
Sin(r32 Angle)
{
    r32 Result = sinf(Angle);
    return(Result);
}

inline r32
Cos(r32 Angle)
{
    r32 Result = cosf(Angle);
    return(Result);
}

inline r32
ATan2(r32 Y, r32 X)
{
    r32 Result = atan2f(Y, X);
    return(Result);
}

struct bit_scan_result
{
    b32 Found;
    u32 Index;
};

inline bit_scan_result
FindLeastSignificantSetBit(u32 Value)
{
    bit_scan_result Result = {};

#if COMPILER_MSVC
    Result.Found = _BitScanForward(&Result.Index, Value);
// TODO: (Kapsy) LLVM!
#else
    for(u32 Test = 0;
            Test < 32;
            ++Test)
    {
        if(Value & (1 << Test))
        {
            Result.Index = Test;
            Result.Found = true;
            break;
        }
    }
#endif

    return(Result);
}

inline u16
ReverseEndianWord(u16 Word)
{
    u8 *Bytes = (u8 *)&Word;
    u16 Result = (Bytes[1] << 0) | (Bytes[0] << 8);
    return(Result);
}

inline u32
ReverseEndianDWord(u32 DWord)
{
    u8 *Bytes = (u8 *)&DWord;
    u32 Result = (Bytes[3] << 0) | (Bytes[2] << 8) | (Bytes[1] << 16) | (Bytes[0] << 24);
    return(Result);
}


////////////////////////////////////////////////////////////////////////////////
//
//  MATHS
//
//  Whole bunch of stuff here ripped directly from Variations.

//
// NOTE: (Kapsy) Scalar operations
//

#define TwoPi32 6.28318530717959f

inline r32
Square(r32 A)
{
    r32 Result = A*A;
    return(Result);
}

inline r32
ClipReal32(r32 Value, r32 Min, r32 Max)
{
    r32 Result = Value;

    if(Value > Max)
    {
        Result = Max;
    }
    else if (Value < Min)
    {
        Result = Min;
    }

    return(Result);
}

inline r32
ClipMinReal32(r32 Value, r32 Min)
{
    r32 Result = Value;

    if (Value < Min)
    {
        Result = Min;
    }

    return(Result);
}

// NOTE: (Kapsy) Linear X-Fade between two input values.
// NOTE: (Kapsy) This is basically a LERP. (Linear intERPolation)
// NOTE: (Kapsy) Amount should be between 0 and 1.
inline r32
XFade(r32 Amount, r32 Input1, r32 Input2)
{
    r32 Result = Input1 + Amount*(Input2-Input1);
    return(Result);
}

// NOTE: (Kapsy) This is the same thing. Still like the XFade way better.
// NOTE: (Kapsy) Amount should be between 0 and 1.
inline r32
Lerp(r32 Amount, r32 Input1, r32 Input2)
{
    r32 Result = (1.f - Amount)*Input1 + Amount*Input2;
    return(Result);
}


inline r32
Clamp(r32 Min, r32 Value, r32 Max)
{
    r32 Result = Value;

    if(Result < Min)
    {
        Result = Min;
    }
    else if(Result > Max)
    {
        Result = Max;
    }

    return(Result);
}

inline s32
ClampS32(s32 Min, s32 Value, s32 Max)
{
    s32 Result = Value;

    if(Result < Min)
    {
        Result = Min;
    }
    else if(Result > Max)
    {
        Result = Max;
    }

    return(Result);
}

inline u32
ClampU32(u32 Min, u32 Value, u32 Max)
{
    u32 Result = Value;

    if(Result < Min)
    {
        Result = Min;
    }
    else if(Result > Max)
    {
        Result = Max;
    }

    return(Result);
}

inline r32
Clamp01(r32 Value)
{
    r32 Result = Clamp(0.0f, Value, 1.0f);

    return(Result);
}

inline r32
Clamp01MapToRange(r32 Min, r32 t, r32 Max)
{
    r32 Result = 0.f;

    r32 Range = Max - Min;
    if(Range != 0.f)
    {
        Result = Clamp01((t - Min)/Range);
    }

    return(Result);
}

// NOTE: (Kapsy) From HMH - not sure if we have a use just yet.
inline r32
SafeRatioN(r32 Numerator, r32 Divisor, r32 N)
{
    r32 Result = N;
    if(Divisor != 0.f)
    {
        Result = Numerator/Divisor;
    }
    return(Result);
}

inline r32
SafeRatio0(r32 Numerator, r32 Divisor)
{
    return SafeRatioN(Numerator, Divisor, 0.f);
}

inline r32
SafeRatio1(r32 Numerator, r32 Divisor)
{
    return SafeRatioN(Numerator, Divisor, 1.f);
}


union u8_4
{
    struct
    {
        u8 a, b, c, d;
    };
    u8 E[4];
};

union value32
{
    u32 U32;
    r32 R32;
    b32 B32;
    u8_4 U8_4;
};

inline value32
Value32(u32 A)
{
    value32 Result;
    Result.U32 = A;
    return(Result);
}

inline value32
Value32(r32 A)
{
    value32 Result;
    Result.R32 = A;
    return(Result);
}

inline value32
Value32(b32 A)
{
    value32 Result;
    Result.B32 = A;
    return(Result);
}

inline value32
Value32(u8_4 A)
{
    value32 Result;
    Result.U8_4 = A;
    return(Result);
}

union value32p
{
    void *Address;
    b32 *B32;
    u32 *U32;
    s32 *S32;
    r32 *R32;
    u8_4 *U8_4;
};

inline value32p
Value32P(void *A)
{
    value32p Result;
    Result.Address = A;
    return(Result);
}

inline value32p
Value32P(u8 *A)
{
    value32p Result;
    Result.Address = (void *)A;
    return(Result);
}

////////////////////////////////////////////////////////////////////////////////
//
//  VECTORS
//
//  Might consider moving these to a seperate h file, because not every project needs them.

//
// NOTE: (Kapsy) v2 operators and functions
//
//
// TODO: (Kapsy) Move to a separate file?
// TODO: (Kapsy) Might even pay to be more specific - variations_vectors.h
union v2
{
    struct
    {
        r32 x, y;
    };
    struct
    {
        r32 u, v;
    };

    r32 E[2];
};

union v3
{
    struct
    {
        r32 x, y, z;
    };
    struct
    {
        r32 u, v, w;
    };
    struct
    {
        r32 r, g, b;
    };
    struct
    {
        v2 xy;
        r32 Ignored0_;
    };
    struct
    {
        r32 Ignored1_;
        v2 yz;
    };
    struct
    {
        v2 uv;
        r32 Ignored2_;
    };
    struct
    {
        r32 Ignored3_;
        v2 vw;
    };

    r32 E[3];
};

union v4
{
    struct
    {
        union
        {
            v3 xyz;
            struct
            {
                r32 x, y, z;
            };
        };

        r32 w;
    };

    struct
    {
        union
        {
            v3 rgb;
            struct
            {
                r32 r, g, b;
            };
        };

        r32 a;
    };

    struct
    {
        v2 xy;
        r32 Ignored0_;
        r32 Ignored1_;
    };
    struct
    {
        r32 Ignored2_;
        v2 yz;
        r32 Ignored3_;
    };
    struct
    {
        r32 Ignored4_;
        r32 Ignored5_;
        v2 zw;
    };
    r32 E[4];
};

// TODO(casey): Consider v2 A = v2{5, 3}; ?
inline v2
V2(r32 A)
{
    v2 Result;

    Result.E[0] = Result.E[1] = A;

    return(Result);
}

inline v2
V2(r32 X, r32 Y)
{
    v2 Result;

    Result.x = X;
    Result.y = Y;

    return(Result);
}

inline v2
V2i(s32 X, s32 Y)
{
    v2 Result;

    Result.x = (r32)X;
    Result.y = (r32)Y;

    return(Result);
}

inline v2
V2i(u32 X, u32 Y)
{
    v2 Result;

    Result.x = (r32)X;
    Result.y = (r32)Y;

    return(Result);
}



inline v3
V3(r32 X, r32 Y, r32 Z)
{
    v3 Result;

    Result.x = X;
    Result.y = Y;
    Result.z = Z;

    return(Result);
}

inline v4
V4(r32 X, r32 Y, r32 Z, r32 W)
{
    v4 Result;

    Result.x = X;
    Result.y = Y;
    Result.z = Z;
    Result.w = W;

    return(Result);
}

inline v3
V3(v2 XY, r32 Z)
{
    v3 Result;

    Result.x = XY.x;
    Result.y = XY.y;
    Result.z = Z;

    return(Result);
}

inline v4
V4(v3 XYZ, r32 W)
{
    v4 Result;

    Result.xyz = XYZ;
    Result.w = W;

    return(Result);
}

//
// NOTE: (Kapsy) v2 Operations.
//

inline v2
Perp(v2 A)
{
    v2 Result = { -A.y, A.x };
    return(Result);
}

inline v2
operator*(r32 A, v2 B)
{
    v2 Result;

    Result.x = A*B.x;
    Result.y = A*B.y;

    return(Result);
}

inline v2
operator*(v2 B, r32 A)
{
    v2 Result = A*B;

    return(Result);
}

inline v2
operator*(v2 A, v2 B)
{
    v2 Result;

    Result.x = A.x*B.x;
    Result.y = A.y*B.y;

    return(Result);
}

inline v2 &
operator*=(v2 &B, r32 A)
{
    B = A * B;
    return(B);
}

inline
v2 operator-(v2 A)
{
    v2 Result;

    Result.x = -A.x;
    Result.y = -A.y;

    return(Result);
}

inline
v2 operator+(v2 A, v2 B)
{
    v2 Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;

    return(Result);
}

inline
v2 operator+(v2 A, r32 B)
{
    v2 Result = A + V2(B, B);

    return(Result);
}

inline v2 &
operator+=(v2 &A, v2 B)
{
    A = A + B;
    return(A);
}

inline
v2 operator-(v2 A, v2 B)
{
    v2 Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;

    return(Result);
}

inline
v2 operator-=(v2 &A, v2 B)
{
    A = A - B;
    return(A);
}

inline v2
Hadamard(v2 A, v2 B)
{
    v2 Result = {A.x*B.x, A.y*B.y};

    return(Result);
}

inline r32
Inner(v2 A, v2 B)
{
    r32 Result = A.x*B.x + A.y*B.y;

    return(Result);
}

// NOTE: (Kapsy) When we get the dot product of a vector with itself, we get it's length squared (because cos at 0 is 1)
inline r32
LengthSq(v2 A)
{
    r32 Result = Inner(A, A);

    return(Result);
}

inline r32
Length(v2 A)
{
    r32 Result = SquareRoot(LengthSq(A));
    return(Result);
}

//
// NOTE: (Kapsy) v3 Operations
//

inline v3
operator*(r32 A, v3 B)
{
    v3 Result;

    Result.x = A*B.x;
    Result.y = A*B.y;
    Result.z = A*B.z;
    
    return(Result);
}

inline v3
operator*(v3 B, r32 A)
{
    v3 Result = A*B;

    return(Result);
}

inline v3 &
operator*=(v3 &B, r32 A)
{
    B = A * B;

    return(B);
}

inline v3
operator-(v3 A)
{
    v3 Result;

    Result.x = -A.x;
    Result.y = -A.y;
    Result.z = -A.z;

    return(Result);
}

inline v3
operator+(v3 A, v3 B)
{
    v3 Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;

    return(Result);
}

inline v3 &
operator+=(v3 &A, v3 B)
{
    A = A + B;

    return(A);
}

inline v3
operator-(v3 A, v3 B)
{
    v3 Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    Result.z = A.z - B.z;

    return(Result);
}

inline v3
Hadamard(v3 A, v3 B)
{
    v3 Result = {A.x*B.x, A.y*B.y, A.z*B.z};

    return(Result);
}

inline r32
Inner(v3 A, v3 B)
{
    r32 Result = A.x*B.x + A.y*B.y + A.z*B.z;

    return(Result);
}

inline r32
LengthSq(v3 A)
{
    r32 Result = Inner(A, A);

    return(Result);
}

inline r32
Length(v3 A)
{
    r32 Result = SquareRoot(LengthSq(A));
    return(Result);
}

inline v3
Normalize(v3 A)
{
    v3 Result = A * (1.0f / Length(A));

    return(Result);
}

inline v3
Clamp01(v3 Value)
{
    v3 Result;

    Result.x = Clamp01(Value.x);
    Result.y = Clamp01(Value.y);
    Result.z = Clamp01(Value.z);

    return(Result);
}

inline v3
Lerp(v3 A, r32 t, v3 B)
{
    v3 Result = (1.f - t)*A + t*B;

    return(Result);
}



//
// NOTE: (Kapsy) v4 Operations
//

inline v4
operator*(r32 A, v4 B)
{
    v4 Result;

    Result.x = A*B.x;
    Result.y = A*B.y;
    Result.z = A*B.z;
    Result.w = A*B.w;

    return(Result);
}

inline v4
operator*(v4 B, r32 A)
{
    v4 Result = A*B;

    return(Result);
}

inline v4 &
operator*=(v4 &B, r32 A)
{
    B = A * B;

    return(B);
}

inline v4
operator-(v4 A)
{
    v4 Result;

    Result.x = -A.x;
    Result.y = -A.y;
    Result.z = -A.z;
    Result.w = -A.w;

    return(Result);
}

inline v4
operator+(v4 A, v4 B)
{
    v4 Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;
    Result.w = A.w + B.w;

    return(Result);
}

inline v4 &
operator+=(v4 &A, v4 B)
{
    A = A + B;

    return(A);
}

inline v4
operator-(v4 A, v4 B)
{
    v4 Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    Result.z = A.z - B.z;
    Result.w = A.w - B.w;

    return(Result);
}

inline v4
Hadamard(v4 A, v4 B)
{
    v4 Result = {A.x*B.x, A.y*B.y, A.z*B.z, A.w*B.w};

    return(Result);
}

inline r32
Inner(v4 A, v4 B)
{
    r32 Result = A.x*B.x + A.y*B.y + A.z*B.z + A.w*B.w;

    return(Result);
}

inline r32
LengthSq(v4 A)
{
    r32 Result = Inner(A, A);

    return(Result);
}

inline r32
Length(v4 A)
{
    r32 Result = SquareRoot(LengthSq(A));
    return(Result);
}

inline v4
Clamp01(v4 Value)
{
    v4 Result;

    Result.x = Clamp01(Value.x);
    Result.y = Clamp01(Value.y);
    Result.z = Clamp01(Value.z);
    Result.w = Clamp01(Value.w);

    return(Result);
}

inline v4
Lerp(v4 A, r32 t, v4 B)
{
    v4 Result = (1.f - t)*A + t*B;

    return(Result);
}

// NOTE: (Kapsy)
// In Photoshop, 255, 0, 0 will become:
// 127, 127, 127 at 0% saturation.
// 159, 95, 95 at 25% saturation.
// 191, 64, 64 at 50% saturation.
// 223, 32, 32 at 75% saturation.
// 255, 255, 255 is unchanged at any saturation value.
inline v4
ColorWithSaturation(v4 C, r32 S)
{
    r32 Avg = (C.r + C.g + C.b)/3.f;
    r32 Weight = 0.6f;
    Avg = Weight + Avg*(1.f-Weight);
    v4 Result = V4(Lerp(V3(Avg, Avg, Avg), S, C.rgb), C.a);
    return(Result);
}

////////////////////////////////////////////////////////////////////////////////
//
//  RECTANGLES
//
//
// NOTE: (Kapsy) rectangle2 operators & functions
//

// TODO: (Kapsy)
// Min = BL, Max = TR, however, when considering text, it makes more sense to use:
// Min = TL, Max = BR, as we want the top y position to be consistant.
// However, it might not really matter that much when you really think about it.
// Want to find out what the standard conventions for 3D etc are.
struct rectangle2
{
    v2 Min;
    v2 Max;
};

inline v2
GetMinCorner(rectangle2 Rect)
{
    v2 Result = Rect.Min;
    return(Result);
}

inline v2
GetMaxCorner(rectangle2 Rect)
{
    v2 Result = Rect.Max;
    return(Result);
}

#if 0
inline v2
GetTopLeft(rectangle2 Rect)
{
    v2 Result = V2(Rect.Min.x, Rect.Max.y);
    return(Result);
}

inline v2
GetBottomRight(rectangle2 Rect)
{
    v2 Result = V2(Rect.Max.x, Rect.Min.y);
    return(Result);
}
#endif

inline v2
GetDim(rectangle2 Rect)
{
    v2 Result = Rect.Max - Rect.Min;
    return(Result);
}

inline v2
GetCenter(rectangle2 Rect)
{
    v2 Result = 0.5f *(Rect.Min + Rect.Max);
    return(Result);
}


inline rectangle2
RectMinMax(v2 Min, v2 Max)
{
    rectangle2 Result;

    Result.Min = Min;
    Result.Max = Max;

    return(Result);
}

inline rectangle2
RectMinDim(v2 Min, v2 Dim)
{
    rectangle2 Result;

    Result.Min = Min;
    Result.Max = Min + Dim;

    return(Result);
}


inline rectangle2
RectCenterHalfDim(v2 Center, v2 HalfDim)
{
    rectangle2 Result;

    Result.Min = Center - HalfDim;
    Result.Max = Center + HalfDim;

    return(Result);
}

inline rectangle2
RectCenterDim(v2 Center, v2 Dim)
{
    rectangle2 Result = RectCenterHalfDim(Center, 0.5f*Dim);

    return(Result);
}

#if 0
inline rectangle2
RectTopLeftDim(v2 TopLeft, v2 Dim)
{
    rectangle2 Result = RectMinDim(V2(TopLeft.x, TopLeft.y - Dim.y), Dim);

    return(Result);
}
#endif

inline rectangle2 &
RoundRect(rectangle2 &A)
{
    RoundReal32(A.Min.x);
    RoundReal32(A.Min.y);
    RoundReal32(A.Max.x);
    RoundReal32(A.Max.y);

    return(A);
}

inline rectangle2
RectUnion(rectangle2 A, rectangle2 B)
{
    rectangle2 Result = RectMinMax(
            V2(((A.Min.x < B.Min.x) ? A.Min.x : B.Min.x),
               ((A.Min.y < B.Min.y) ? A.Min.y : B.Min.y)),
            V2(((A.Max.x > B.Max.x) ? A.Max.x : B.Max.x),
               ((A.Max.y > B.Max.y) ? A.Max.y : B.Max.y))
            );

    return(Result);
}

#if 0
inline rectangle2 &
RectSetBottomRight(rectangle2 &A, v2 BottomRight)
{
    A.Max.x = BottomRight.x;
    A.Min.y = BottomRight.y;

    return(A);
}
#endif

inline b32
IsInRectangle(rectangle2 Rectangle, v2 Test)
{
    b32 Result =
        ((Test.x >= Rectangle.Min.x) &&
         (Test.y >= Rectangle.Min.y) &&
         (Test.x < Rectangle.Max.x) &&
         (Test.y < Rectangle.Max.y));


    return(Result);
}

inline b32
RectanglesIntersect(rectangle2 A, rectangle2 B)
{
    b32 Result =
        !((A.Min.x > B.Max.x) ||
          (A.Max.x < B.Min.x) ||
          (A.Min.y > B.Max.y) ||
          (A.Max.y < B.Min.y));

    return(Result);
}


inline v2
GetBarycentric(rectangle2 A, v2 P)
{
    v2 Result;

    Result.x = SafeRatio0(P.x - A.Min.x, A.Max.x - A.Min.x);
    Result.y = SafeRatio0(P.y - A.Min.y, A.Max.y - A.Min.y);

    return(Result);
}

inline v2
GetPointRelativeMin(rectangle2 A, v2 P)
{
    v2 Result = P - A.Min;

    return(Result);
}

//
// NOTE: (Kapsy) rectangle3 operators & functions
//

struct rectangle3
{
    v3 Min;
    v3 Max;
};

inline v3
GetMinCorner(rectangle3 Rect)
{
    v3 Result = Rect.Min;
    return(Result);
}

inline v3
GetMaxCorner(rectangle3 Rect)
{
    v3 Result = Rect.Max;
    return(Result);
}

inline v3
GetDim(rectangle3 Rect)
{
    v3 Result = Rect.Max - Rect.Min;
    return(Result);
}

inline v3
GetCenter(rectangle3 Rect)
{
    v3 Result = 0.5f*(Rect.Min + Rect.Max);
    return(Result);
}

inline rectangle3
RectMinMax(v3 Min, v3 Max)
{
    rectangle3 Result;

    Result.Min = Min;
    Result.Max = Max;

    return(Result);
}

inline rectangle3
RectMinDim(v3 Min, v3 Dim)
{
    rectangle3 Result;

    Result.Min = Min;
    Result.Max = Min + Dim;

    return(Result);
}

inline rectangle3
RectCenterHalfDim(v3 Center, v3 HalfDim)
{
    rectangle3 Result;

    Result.Min = Center - HalfDim;
    Result.Max = Center + HalfDim;

    return(Result);
}

inline rectangle3
AddRadiusTo(rectangle3 A, v3 Radius)
{
    rectangle3 Result;

    Result.Min = A.Min - Radius;
    Result.Max = A.Max + Radius;

    return(Result);
}

inline rectangle3
Offset(rectangle3 A, v3 Offset)
{
    rectangle3 Result;

    Result.Min = A.Min + Offset;
    Result.Max = A.Max + Offset;

    return(Result);
}

inline rectangle3
RectCenterDim(v3 Center, v3 Dim)
{
    rectangle3 Result = RectCenterHalfDim(Center, 0.5f*Dim);

    return(Result);
}

inline b32
IsInRectangle(rectangle3 Rectangle, v3 Test)
{
    b32 Result = ((Test.x >= Rectangle.Min.x) &&
                     (Test.y >= Rectangle.Min.y) &&
                     (Test.z >= Rectangle.Min.z) &&
                     (Test.x < Rectangle.Max.x) &&
                     (Test.y < Rectangle.Max.y) &&
                     (Test.z < Rectangle.Max.z));

    return(Result);
}

inline b32
RectanglesIntersect(rectangle3 A, rectangle3 B)
{
    b32 Result = !((B.Max.x <= A.Min.x) ||
                      (B.Min.x >= A.Max.x) ||
                      (B.Max.y <= A.Min.y) ||
                      (B.Min.y >= A.Max.y) ||
                      (B.Max.z <= A.Min.z) ||
                      (B.Min.z >= A.Max.z));
    return(Result);
}

inline v3
GetBarycentric(rectangle3 A, v3 P)
{
    v3 Result;

    Result.x = SafeRatio0(P.x - A.Min.x, A.Max.x - A.Min.x);
    Result.y = SafeRatio0(P.y - A.Min.y, A.Max.y - A.Min.y);
    Result.z = SafeRatio0(P.z - A.Min.z, A.Max.z - A.Min.z);

    return(Result);
}

inline rectangle2
ToRectangleXY(rectangle3 A)
{
    rectangle2 Result;

    Result.Min = A.Min.xy;
    Result.Max = A.Max.xy;

    return(Result);
}

//
// NOTE: (Kapsy) rectangle2i stuff
//

struct rectangle2i
{
    s32 MinX, MinY;
    s32 MaxX, MaxY;
};

inline rectangle2i
Intersect(rectangle2i A, rectangle2i B)
{
    rectangle2i Result;

    Result.MinX = (A.MinX < B.MinX) ? B.MinX : A.MinX;
    Result.MinY = (A.MinY < B.MinY) ? B.MinY : A.MinY;
    Result.MaxX = (A.MaxX > B.MaxX) ? B.MaxX : A.MaxX;
    Result.MaxY = (A.MaxY > B.MaxY) ? B.MaxY : A.MaxY;

    return(Result);
}

inline rectangle2i
Union(rectangle2i A, rectangle2i B)
{
    rectangle2i Result;

    Result.MinX = (A.MinX < B.MinX) ? A.MinX : B.MinX;
    Result.MinY = (A.MinY < B.MinY) ? A.MinY : B.MinY;
    Result.MaxX = (A.MaxX > B.MaxX) ? A.MaxX : B.MaxX;
    Result.MaxY = (A.MaxY > B.MaxY) ? A.MaxY : B.MaxY;

    return(Result);
}

inline s32
GetClampedRectArea(rectangle2i A)
{
    s32 Width = (A.MaxX - A.MinX);
    s32 Height = (A.MaxY - A.MinY);
    s32 Result = 0;
    if((Width > 0) && (Height > 0))
    {
        Result = Width*Height;
    }

    return(Result);
}

inline b32
HasArea(rectangle2i A)
{
    b32 Result = ((A.MinX < A.MaxX) && (A.MinY < A.MaxY));

    return(Result);
}

inline rectangle2i
InvertedInfinityRectangle(void)
{
    rectangle2i Result;

    Result.MinX = Result.MinY = INT_MAX;
    Result.MaxX = Result.MaxY = -INT_MAX;

    return(Result);
}


////////////////////////////////////////////////////////////////////////////////
//
//  MUSICAL TIME
//
//  TODO: (Kapsy) Might remove the operators here and make these more c-like.

struct musical_time
{
    s32 Beats;
    r32 SubBeats;
};


inline musical_time
CanonicalizeTime(musical_time A)
{
    // NOTE: (Kapsy) Ensures that -1.0f > SubBeats < 1.0f
    s32 Whole = (s32)A.SubBeats;
    A.Beats += Whole;
    A.SubBeats -= (r32)Whole;

    // TODO: (Kapsy) I'm sure that there is a more efficient way to do this!
    if((A.Beats > 0) && (A.SubBeats < 0.0f))
    {
        A.Beats -= 1;
        A.SubBeats += 1.0f;
    }
    else if ((A.Beats < 0) && (A.SubBeats > 0.0f))
    {
        A.Beats += 1;
        A.SubBeats -= 1.0f;
    }

    // TODO: (Kapsy) Surround with a debug only macro.
    if(A.Beats > 0)
    {

        Assert(A.SubBeats >= 0.0f);
    }
    if(A.Beats < 0)
    {
        Assert(A.SubBeats <= 0.0f);
    }

    return(A);
}

inline musical_time
MusicalTime(s32 Beats, r32 SubBeats)
{
    musical_time Result;

    Result.Beats = Beats;
    Result.SubBeats = SubBeats;

    return(CanonicalizeTime(Result));
}

inline musical_time
MusicalTime(r32 RealBeats)
{
    musical_time Result;

    Result.Beats = 0;
    Result.SubBeats = RealBeats;

    return(CanonicalizeTime(Result));
}

inline
musical_time operator+(musical_time A, musical_time B)
{
    musical_time Result;

    Result.Beats = (A.Beats + B.Beats);
    Result.SubBeats = (A.SubBeats + B.SubBeats);

    return(CanonicalizeTime(Result));
}

inline musical_time &
operator+=(musical_time &A, musical_time B)
{
    A = A + B;
    return(A);
}

inline musical_time &
operator+=(musical_time &A, r32 B)
{
    A = A + MusicalTime(B);
    return(A);
}
inline
musical_time operator-(musical_time A, musical_time B)
{
    musical_time Result;

    Result.Beats = (A.Beats - B.Beats);
    Result.SubBeats = (A.SubBeats - B.SubBeats);

    return(CanonicalizeTime(Result));
}

inline
musical_time operator-(musical_time A, r32 B)
{
    musical_time Result = A - MusicalTime(B);

    return(Result);
}

inline musical_time &
operator-=(musical_time &A, musical_time B)
{
    A = A - B;
    return(A);
}

inline musical_time &
operator-=(musical_time &A, r32 B)
{
    A = A - B;
    return(A);
}

inline
musical_time operator*(musical_time A, r32 B)
{
    musical_time Result;

    r32 Beats = (r32)A.Beats*B;
    r32 SubBeats = A.SubBeats*B;

    Result = MusicalTime(Beats) + MusicalTime(SubBeats);

    return(CanonicalizeTime(Result));
}

inline
musical_time &
operator*=(musical_time &A, r32 B)
{
    A = A * B;
    return(A);
}

inline
b32 operator>(musical_time A, musical_time B)
{
    b32 Result;

    if(A.Beats > B.Beats)
    {
        Result = true;
    }
    else if((A.Beats == B.Beats) && (A.SubBeats > B.SubBeats))
    {
        Result = true;
    }
    else
    {
        Result = false;
    }

    return(Result);
}

inline
b32 operator>=(musical_time A, musical_time B)
{
    b32 Result;

    if(A.Beats > B.Beats)
    {
        Result = true;
    }
    else if((A.Beats == B.Beats) && (A.SubBeats >= B.SubBeats))
    {
        Result = true;
    }
    else
    {
        Result = false;
    }

    return(Result);
}

inline
b32 operator<(musical_time A, musical_time B)
{
    b32 Result;

    if(A.Beats < B.Beats)
    {
        Result = true;
    }
    else if((A.Beats == B.Beats) && (A.SubBeats < B.SubBeats))
    {
        Result = true;
    }
    else
    {
        Result = false;
    }

    return(Result);
}

inline
b32 operator<=(musical_time A, musical_time B)
{
    b32 Result;

    if(A.Beats < B.Beats)
    {
        Result = true;
    }
    else if((A.Beats == B.Beats) && (A.SubBeats <= B.SubBeats))
    {
        Result = true;
    }
    else
    {
        Result = false;
    }

    return(Result);
}

inline musical_time
SecondsToMusicalTime(r32 BeatsPerMinute, r32 Seconds)
{
    musical_time Result;

    r32 SecondsPerBeat = 60.0f/BeatsPerMinute;
    r32 RealBeats = Seconds/SecondsPerBeat;

    Result = MusicalTime(RealBeats);

    return(Result);
}

inline r32
MusicalTimeToSeconds(r32 BeatsPerMinute, musical_time Time)
{
    r32 Result;

    // TODO: (Kapsy) Make SECONDS_PER_MIN constant?
    r32 SecondsPerBeat = 60.0f/BeatsPerMinute;

    Result = ((Time.Beats + Time.SubBeats)*SecondsPerBeat);

    return(Result);
}

inline r32
MusicalTimeToReal32(musical_time A)
{
    return((r32)A.Beats + A.SubBeats);
}

////////////////////////////////////////////////////////////////////////////////
//
//  AUDIO FUNCTIONS
//
//  TODO: (Kapsy) Separate file?
//

inline r32
SemitonesToHz(r32 Semitones)
{
    r32 Result = Pow64(2.f, (Semitones - 69.f) / 12.f) * 440.f;

    return(Result);
}

inline r32
LinearToHz(r32 Amount, r32 MaxHz)
{
    Assert(Amount <= 1.f && Amount >= 0.f);

    r32 Result = Amount*Amount*Amount*Amount*MaxHz;

    return(Result);
}

//
// NOTE: (Kapsy) sRGB functions
//

inline v4
SRGB255ToLinear1(v4 C)
{
    v4 Result;

    r32 Inv255 = 1.f / 255.f;

    Result.r = Square(Inv255*C.r);
    Result.g = Square(Inv255*C.g);
    Result.b = Square(Inv255*C.b);
    Result.a = Inv255*C.a;

    return(Result);
}

inline v4
Linear1ToSRGB255(v4 C)
{
    v4 Result;

    r32 One255 = 255.f;


    Result.r = One255*SquareRoot(C.r);
    Result.g = One255*SquareRoot(C.g);
    Result.b = One255*SquareRoot(C.b);
    Result.a = One255*C.a;

    return(Result);
}

#define KAPSY_H
#endif
