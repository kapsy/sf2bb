#include <stdio.h>
#include <stdlib.h>
#include "kapsy.h"
#include "midi2note.h"

///////////////////////////////////////////////////////////////////////////////
// Arg Handling ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
enum
{
    sf2bbArg_BPM,
    sf2bbArg_Input,
    sf2bbArg_Divisions,
    sf2bbArg_DivisionsStart,
    sf2bbArg_MonoLeft,
    sf2bbArg_MonoRight,
    sf2bbArg_Normalize,
};

char *ArgDefs[] =
{
    "--bpm",
    "--input",
    "--divisions",
    "--divisions-start",
    "--monoleft",
    "--monoright",
    "--normalize",
};
static int ArgDefsCount;

#define MAX_COLLATED_ARG_VALUES 16

struct collated_arg
{
    int Found;
    char *Values[MAX_COLLATED_ARG_VALUES];
    int ValueCount;
};

#define MAX_ARG_RESULTS 16

struct collated_args
{
    collated_arg Args[MAX_ARG_RESULTS];
    int Count;
};

static void
kapsy_PrintCollatedArgs(collated_args *Args)
{
    for(int ArgIndex = 0;
            ArgIndex < ArrayCount(Args->Args);
            ++ArgIndex)
    {
        collated_arg *Arg = Args->Args + ArgIndex;
        if(Arg->Found)
        {
            printf("ArgIndex: %d\n", ArgIndex);
            for(int ValueIndex = 0;
                    ValueIndex < Arg->ValueCount;
                    ++ValueIndex)
            {
                printf("Value: %s\n", Arg->Values[ValueIndex]);
            }
            printf("\n");
        }
    }
}

internal void
kapsy_GetCollatedArgs(int ArgCount, char **Args,
        char **Defs, int DefsCount,
        collated_args *Result)
{
    collated_arg *CurrentArg = 0;

    for(int ArgIndex = 1;
            ArgIndex < ArgCount;
            ++ArgIndex)
    {
        char *ArgRaw = *(Args + ArgIndex);
        int ArgIsDef = 0;

        for(int DefIndex = 0;
                DefIndex < DefsCount;
                ++DefIndex)
        {
            char *Def = *(Defs + DefIndex);
            if(kapsy_StringComp(Def, ArgRaw))
            {
                CurrentArg = Result->Args + DefIndex;
                CurrentArg->Found = 1;
                ArgIsDef = 1;
                break;
            }
        }

        if(!ArgIsDef && CurrentArg)
        {
            char **CurrentArgValue = (CurrentArg->Values + CurrentArg->ValueCount++);
            *CurrentArgValue = ArgRaw;
        }
    }

}

///////////////////////////////////////////////////////////////////////////////
// WAV Specs //////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// TODO: (KAPSY) Let's move all this to kapsy.io
struct read_file_result
{
    u32 ContentsSize;
    void *Contents;
};

internal read_file_result
ReadEntireFile2(char *Filename)
{
    read_file_result Result = {};

    FILE *File = fopen(Filename, "r");

    if(File)
    {
        fseek(File, 0, SEEK_END);
        u32 ContentsSize = ftell(File);
        fseek(File, 0, SEEK_SET);

        Result.Contents = (void *)malloc(ContentsSize);
        Result.ContentsSize = ContentsSize;
        u64 ReadBytes = fread(Result.Contents, Result.ContentsSize, 1, File);

        // Assert(ReadBytes == Result.ContentsSize);
    }

    return(Result);
}

#define HI_NIBBLE 0xF0
#define LO_NIBBLE 0x0F
#define WAV_FMT_CHUNK_ID 0x666D7420
#define WAV_DATA_CHUNK_ID 0x64617461

#pragma pack(push, 1)
struct wav_sub_chunk
{
    u32 ID; // Big
    u32 Size;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct wav_header
{
    // NOTE: (Kapsy) All x86 CPUs are Little Endian!
    u32 ChunkID; // Big
    u32 ChunkSize;
    u32 Format; // Big

    // u32 SubChunk1ID; // Big
    // u32 SubChunk1Size;
    // u16 AudioFormat;
    // u16 NumChannels;
    // u32 SampleRate;
    // u32 ByteRate;
    // u16 BlockAlign;
    // u16 BitsPerSample;
    // u32 SubChunk2ID; // Big
    // u32 SubChunk2Size;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct wav_out_header
{
    // NOTE: (Kapsy) All x86 CPUs are Little Endian!
    u32 ChunkID; // Big
    u32 ChunkSize;
    u32 Format; // Big

    u32 SubChunk1ID; // Big
    u32 SubChunk1Size;
    u16 AudioFormat;
    u16 NumChannels;
    u32 SampleRate;
    u32 ByteRate;
    u16 BlockAlign;
    u16 BitsPerSample;
    u32 SubChunk2ID; // Big
    u32 SubChunk2Size;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct wav_fmt
{
    u16 AudioFormat;
    u16 NumChannels;
    u32 SampleRate;
    u32 ByteRate;
    u16 BlockAlign;
    u16 BitsPerSample;
    u32 SubChunk2ID; // Big
    u32 SubChunk2Size;
};
#pragma pack(pop)

///////////////////////////////////////////////////////////////////////////////
// SF2 Spec Structs ///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// NOTE: (KAPSY) Four Character Code
typedef short SHORT;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned char BYTE;
typedef char CHAR;
typedef DWORD FOURCC;

typedef WORD SFModulator;
typedef WORD SFGenerator;
typedef WORD SFTransform;

#define MAX_CHILDREN_CHUNKS (1 << 4)
#define MAX_CHUNK_POOL_CHUNKS (1 << 11)


struct chunk
{
    FOURCC ckID;
    DWORD ckSize;
    BYTE *ckDATA;

    u32 IsList;

    chunk *Children;
    u32 ChildrenCount;
};

global_variable chunk ChunkPool[MAX_CHUNK_POOL_CHUNKS];
global_variable u32 ChunkPoolPosition;

#if 0
struct chunk
{
    FOURCC ckHeader;
    FOURCC ckID;
    DWORD ckSize;
};
#endif


#pragma pack(push, 1)
struct sfVersionTag
{
    WORD wMajor;
    WORD wMinor;
};

struct sfPresetHeader
{
    CHAR achPresetName[20];
    WORD wPreset;
    WORD wBank;
    WORD wPresetBagNdx;
    DWORD dwLibrary;
    DWORD dwGenre;
    DWORD dwMorphology;
};

struct sfPresetBag
{
    WORD wGenNdx;
    WORD wModNdx;
};

struct sfModList
{
    SFModulator sfModSrcOper;
    SFGenerator sfModDestOper;
    SHORT modAmount;
    SFModulator sfModAmtSrcOper;
    SFTransform sfModTransOper;
};

struct rangesType
{
    BYTE byLo;
    BYTE byHi;
};

union genAmountType
{
    rangesType ranges;
    SHORT shAmount;
    WORD wAmount;
};

struct sfGenList
{
    SFGenerator sfGenOper;
    genAmountType genAmount;
};

struct sfInst
{
    CHAR achInstName[20];
    WORD wInstBagNdx;
};

struct sfInstBag
{
    WORD wInstGenNdx;
    WORD wInstModNdx;
};

// TODO: (KAPSY) Do we need a separate struct here?
struct sfInstGenList
{
    SFGenerator sfGenOper;
    genAmountType genAmount;
};

enum
{
    monoSample = 1,
    rightSample = 2,
    leftSample = 4,
    linkedSample = 8,
    RomMonoSample = 0x8001,
    RomRightSample = 0x8002,
    RomLeftSample = 0x8004,
    RomLinkedSample = 0x8008
};

typedef WORD SFSampleLink;

struct sfSample
{
    CHAR achSampleName[20];
    DWORD dwStart;
    DWORD dwEnd;
    DWORD dwStartloop;
    DWORD dwEndloop;
    DWORD dwSampleRate;
    BYTE byOriginalPitch;
    CHAR chPitchCorrection;
    WORD wSampleLink;
    SFSampleLink sfSampleType;
};
#pragma pack(pop)


// 2017年 9月 5日 火曜日 22時38分14秒 JST
// SO, I'm pretty sure the reason we aren't working is that we haven't created
// the universal inst zone. We only have one.


enum
{
    startAddrsOffset = 0,
    endAddrsOffset = 1,
    startloopAddrsOffset = 2,
    endloopAddrsOffset = 3,
    startAddrsCoarseOffset = 4,
    modLfoToPitch = 5,
    vibLfoToPitch = 6,
    modEnvToPitch = 7,
    initialFilterFc = 8,
    initialFilterQ = 9,
    modLfoToFilterFc = 10,
    modEnvToFilterFc = 11,
    endAddrsCoarseOffset = 12,
    modLfoToVolume = 13,
    chorusEffectsSend = 15,
    reverbEffectsSend = 16,
    pan = 17,
    delayModLFO = 21,
    freqModLFO = 22,
    delayVibLFO = 23,
    freqVibLFO = 24,
    delayModEnv = 25,
    attackModEnv = 26,
    holdModEnv = 27,
    decayModEnv = 28,
    sustainModEnv = 29,
    releaseModEnv = 30,
    keynumToModEnvHold = 31,
    keynumToModEnvDecay = 32,
    delayVolEnv = 33,
    attackVolEnv = 34,
    holdVolEnv = 35,
    decayVolEnv = 36,
    sustainVolEnv = 37,
    releaseVolEnv = 38,
    keynumToVolEnvHold = 39,
    keynumToVolEnvDecay = 40,
    instrument = 41,
    reserved1 = 42,
    keyRange = 43,
    velRange = 44,
    startloopAddrsCoarseOffset = 45,
    keynum= 46,
    velocity = 47,
    initialAttenuation = 48,
    endloopAddrsCoarseOffset = 50,
    coarseTune = 51,
    fineTune = 52,
    sampleID = 53,
    sampleModes = 54,
    scaleTuning = 56,
    exclusiveClass = 57,
    overridingRootKey = 58,
};

///////////////////////////////////////////////////////////////////////////////
// Internal SF2 Defs //////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define MAX_PRESETS 16
#define MAX_PRESET_MODULATORS 16
#define MAX_PRESET_GENERATORS 16
#define MAX_INSTRUMENTS 16
#define MAX_INST_BAGS 128
#define MAX_INST_MODULATORS 1024
#define MAX_INST_GENERATORS 1024
#define MAX_SAMPLES 32

struct sf_sample_params
{
    char *Path;
};

struct sf_inst_params
{
    char *Name;
};

struct sf_preset_params
{
    char *Name;
};

struct sf_zone_params
{
    int InstrumentIndex;
    int SampleIndex;
    int SampleStart;
    int SampleEnd;
    int Key;
    float TuningOffset;
};

// NOTE: (KAPSY) All Create() functions are directly translated to the
// appropriate SF2 structs for file writing. For a more complete editor we
// would store everything in an editable state.
struct sf_prestate
{
    u32 SampleDataCount;

    BYTE *SampleData16;
    BYTE *SampleData24;

    // TODO: (Kapsy) Need to add the following lists of data for SF2 files:
    // (all lists)
    sfPresetHeader Presets[MAX_PRESETS];
    int PresetCount;

    sfPresetBag PresetBags[MAX_PRESETS];
    int PresetBagCount;

    sfModList PresetModulators[MAX_PRESET_MODULATORS];
    int PresetModulatorCount;

    sfGenList PresetGenerators[MAX_PRESET_GENERATORS];
    int PresetGeneratorCount;

    sfInst Instruments[MAX_INSTRUMENTS];
    int InstrumentCount;

    sfInstBag InstrumentBags[MAX_INST_BAGS];
    int InstrumentBagCount;

    sfModList InstrumentModulators[MAX_INST_MODULATORS];
    int InstrumentModulatorCount;

    sfInstGenList InstrumentGenerators[MAX_INST_GENERATORS];
    int InstrumentGeneratorCount;

    sfSample Samples[MAX_SAMPLES];
    int SampleCount;
};

internal int
CreateTestSample(sf_prestate *Prestate)
{
    int BytesPerSample24 = 1;
    int BytesPerSample16 = 2;

    u32 PaddingCount = 46;
    u32 FrameCount = 2;
    u32 FrameCountPadded = FrameCount + PaddingCount;

    // BYTE *SampleOut24 =
        // (Prestate->SampleData24 + Prestate->SampleDataCount*BytesPerSample24);

    BYTE *NewSampleData16 = (BYTE *)realloc(Prestate->SampleData16,
            Prestate->SampleDataCount + FrameCountPadded*BytesPerSample16);
    if(NewSampleData16)
    {
        Prestate->SampleData16 = NewSampleData16;
    }
    else
    {
        Assert(!"Unable to allocate sample memory, aborting.\n");
    }


    BYTE *SampleOut16 =
        (Prestate->SampleData16 + Prestate->SampleDataCount*BytesPerSample16);

    /* *SampleOut24++ = 0xFF;
    *SampleOut24++ = 0x0; */

    *SampleOut16++ = 0x0;
    *SampleOut16++ = 0x0;

    *SampleOut16++ = 0xFF;
    *SampleOut16++ = 0xFF;

    for(u32 PaddingIndex = 0;
            PaddingIndex < PaddingCount;
            ++PaddingIndex)
    {
        *SampleOut16++ = 0x0;
    }


    int IndexLeft = Prestate->SampleCount++;
    sfSample *SampleLeft = Prestate->Samples + IndexLeft;

    sprintf(SampleLeft->achSampleName, "%s", "sf2_test_wav");
    SampleLeft->dwStart = Prestate->SampleDataCount; // Offset from the sample data start.
    SampleLeft->dwEnd = Prestate->SampleDataCount + FrameCount;
    SampleLeft->dwStartloop = 0;
    SampleLeft->dwEndloop = 0;
    SampleLeft->dwSampleRate = 44100;
    SampleLeft->byOriginalPitch = 41;
    SampleLeft->chPitchCorrection = 0;
    SampleLeft->wSampleLink = 0;
    SampleLeft->sfSampleType = monoSample;

    Prestate->SampleDataCount+=FrameCountPadded;
    return(IndexLeft);
}

internal int
CreateSample(sf_prestate *Prestate, char *Path)
{
    read_file_result WavFileResult = ReadEntireFile2(Path);

    Assert(WavFileResult.ContentsSize != 0);

    u8 *ReadPosition = (u8 *)WavFileResult.Contents;
    u8 *FileEnd = ReadPosition + WavFileResult.ContentsSize;

    wav_header *WavHeader = (wav_header *)WavFileResult.Contents;
    ReadPosition+= sizeof(*WavHeader);

    wav_fmt *Format = 0;
    wav_sub_chunk *DataChunk = 0;

    while(ReadPosition < FileEnd)
    {
        wav_sub_chunk *SubChunk = (wav_sub_chunk *)ReadPosition;
        switch(ReverseEndianDWord(SubChunk->ID))
        {
            case WAV_FMT_CHUNK_ID:
                {
                    Format = (wav_fmt *)(ReadPosition + sizeof(*SubChunk));
                } break;
            case WAV_DATA_CHUNK_ID:
                {
                    DataChunk = (wav_sub_chunk *)ReadPosition;
                } break;
        }
        ReadPosition+=(sizeof(*SubChunk) + SubChunk->Size);
    }

    Assert(Format);
    Assert(DataChunk);
    // TODO: (KAPSY) Up to here should be a common function in kapsy.h

    u32 PaddingCount = 46;
    // NOTE: (Kapsy) Format->BlockAlign is 6 for stereo 24 bit sample.
    u32 FrameCount = (DataChunk->Size/Format->BlockAlign);
    u32 FrameCountPadded = FrameCount + PaddingCount;
    u32 SampleCountPadded = FrameCountPadded*Format->NumChannels;

    Assert((Format->NumChannels == 1) || (Format->NumChannels == 2));

    int BytesPerSample24 = 1;
    int BytesPerSample16 = 2;

    BYTE *NewSampleData24 = (BYTE *)realloc(Prestate->SampleData24,
            Prestate->SampleDataCount + SampleCountPadded*BytesPerSample24);
    if(NewSampleData24)
    {
        Prestate->SampleData24 = NewSampleData24;
    }
    else
    {
        fprintf(stderr, "Unable to allocate sample memory, aborting.\n");
        return(-1);
    }

    BYTE *NewSampleData16 = (BYTE *)realloc(Prestate->SampleData16,
            Prestate->SampleDataCount + SampleCountPadded*BytesPerSample16);
    if(NewSampleData16)
    {
        Prestate->SampleData16 = NewSampleData16;
    }
    else
    {
        fprintf(stderr, "Unable to allocate sample memory, aborting.\n");
        return(-1);
    }

    BYTE *SampleIn = (BYTE *)(DataChunk + 1);
    BYTE *SampleOut24 =
        (Prestate->SampleData24 + Prestate->SampleDataCount*BytesPerSample24);
    BYTE *SampleOut16 =
        (Prestate->SampleData16 + Prestate->SampleDataCount*BytesPerSample16);

    for(u32 FrameIndex = 0;
            FrameIndex < FrameCount;
            ++FrameIndex)
    {
        *SampleOut24++ = *SampleIn++;
        *SampleOut16++ = *SampleIn++;
        *SampleOut16++ = *SampleIn++;

        // TODO: (KASPY) Mono file support.
        *SampleOut24++ = *SampleIn++;
        *SampleOut16++ = *SampleIn++;
        *SampleOut16++ = *SampleIn++;
    }

    int IndexLeft = Prestate->SampleCount++;
    sfSample *SampleLeft = Prestate->Samples + IndexLeft;

    int IndexRight = Prestate->SampleCount++;
    sfSample *SampleRight = Prestate->Samples + IndexRight;

    sprintf(SampleLeft->achSampleName, "%s", "TestSample");
    SampleLeft->dwStart = Prestate->SampleDataCount; // Offset from the sample data start.
    SampleLeft->dwEnd = Prestate->SampleDataCount + FrameCount;
    SampleLeft->dwStartloop = 0;
    SampleLeft->dwEndloop = 0;
    SampleLeft->dwSampleRate = Format->SampleRate;
    SampleLeft->byOriginalPitch = 255;
    SampleLeft->chPitchCorrection = 0;
    // NOTE: (KAPSY) Index for the sample of the other channel -
    // interleaved not supported!
    SampleLeft->wSampleLink = IndexRight;
    SampleLeft->sfSampleType = leftSample;
    Prestate->SampleDataCount+=FrameCountPadded;


    sprintf(SampleRight->achSampleName, "%s", "TestSample");
    SampleRight->dwStart = Prestate->SampleDataCount; // Offset from the sample data start.
    SampleRight->dwEnd = Prestate->SampleDataCount + FrameCount;
    SampleRight->dwStartloop = 0;
    SampleRight->dwEndloop = 0;
    SampleRight->dwSampleRate = Format->SampleRate;
    SampleRight->byOriginalPitch = 255;
    SampleRight->chPitchCorrection = 0;
    SampleRight->wSampleLink = 0;
    SampleRight->sfSampleType = rightSample;
    Prestate->SampleDataCount+=FrameCountPadded;

    return(IndexLeft);
}

internal void
CreateInstGenerator(sf_prestate *Prestate, int GenOper, genAmountType GenAmount)
{
    sfInstGenList *Generator = Prestate->InstrumentGenerators +
        Prestate->InstrumentGeneratorCount++;

    Generator->sfGenOper = GenOper;
    Generator->genAmount = GenAmount;
}

internal void
CreatePresetGenerator(sf_prestate *Prestate, int GenOper, genAmountType GenAmount)
{
    sfGenList *Generator = Prestate->PresetGenerators +
        Prestate->PresetGeneratorCount++;

    Generator->sfGenOper = GenOper;
    Generator->genAmount = GenAmount;
}

internal int
CreateInst(sf_prestate *Prestate, sf_inst_params Params)
{
    Assert(Prestate->InstrumentCount < ArrayCount(Prestate->Instruments));
    int Index = Prestate->InstrumentCount++;
    sfInst *Inst = Prestate->Instruments + Index;

    Assert(StringLength(Params.Name) < ArrayCount(Inst->achInstName));
    sprintf(Inst->achInstName, "%s", Params.Name);

    // NOTE: (KAPSY) Index to the instruments zone list.
    Inst->wInstBagNdx = Prestate->InstrumentBagCount;

    return(Index);
}

internal int
CreatePreset(sf_prestate *Prestate, sf_preset_params Params)
{
    Assert(Prestate->PresetCount < ArrayCount(Prestate->Presets));

    int Index = Prestate->PresetCount++;
    sfPresetHeader *Preset = Prestate->Presets + Index;

    Assert(StringLength(Params.Name) < ArrayCount(Preset->achPresetName));
    sprintf(Preset->achPresetName, "%s", Params.Name);

    // NOTE: (KAPSY) Index to the instruments zone list.
    // Preset->wPresetBagNdx = Prestate->PresetrumentBagCount;
    //
    //
    //

    Assert(Prestate->PresetBagCount < ArrayCount(Prestate->PresetBags));
    sfPresetBag *PresetBag =
        Prestate->PresetBags + Prestate->PresetBagCount++;
    ZeroStruct(*PresetBag);

    Assert(Prestate->PresetBagCount < ArrayCount(Prestate->PresetBags));
    sfPresetBag *PresetBag2 =
        Prestate->PresetBags + Prestate->PresetBagCount++;
    ZeroStruct(*PresetBag2);


    genAmountType Ranges = { .ranges = { 0, 0x7f } };
    CreatePresetGenerator(Prestate, keyRange, Ranges);

    genAmountType InstrumentIndex = { .wAmount = 0 };
    CreatePresetGenerator(Prestate, instrument, InstrumentIndex);

    return(Index);
}

internal int
CreateGlobalZone(sf_prestate *Prestate)
{
    int InstBagCount = Prestate->InstrumentBagCount++;
    Assert(InstBagCount < ArrayCount(Prestate->InstrumentBags));
    sfInstBag *InstBag = Prestate->InstrumentBags + InstBagCount;

    InstBag->wInstGenNdx = Prestate->InstrumentGeneratorCount;
    InstBag->wInstModNdx = Prestate->InstrumentModulatorCount;

    genAmountType KeyRange = { .ranges = { 0, 0x7f } };
    CreateInstGenerator(Prestate, keyRange, KeyRange);

    genAmountType VelRange = { .ranges = { 0, 0x7f } };
    CreateInstGenerator(Prestate, velRange, VelRange);

    return(InstBagCount);
}

// TODO: (KAPSY) Are we even able to provide an index here?
// Should be renamed to CreateInstZone
internal int
CreateZone(sf_prestate *Prestate, sf_zone_params Params)
{
    int InstrumentIndex = Params.InstrumentIndex;
    sfInst *Inst = Prestate->Instruments + InstrumentIndex;

    // TODO: (KAPSY) So these are zones, and we have to create at least as many
    // as we have hits for.
    int InstBagCount = Prestate->InstrumentBagCount++;
    Assert(InstBagCount < ArrayCount(Prestate->InstrumentBags));
    sfInstBag *InstBag = Prestate->InstrumentBags + InstBagCount;

    InstBag->wInstGenNdx = Prestate->InstrumentGeneratorCount;
    InstBag->wInstModNdx = Prestate->InstrumentModulatorCount;

    // CreateInstGenerator(Prestate, startAddrsOffset, Params.SampleStart);
    // CreateInstGenerator(Prestate, endAddrsOffset, Params.SampleEnd);

    // TODO: (KAPSY) Proper conversion functions for these times.
    // CreateInstGenerator(Prestate, delayVolEnv, -32768);
    // CreateInstGenerator(Prestate, attackVolEnv, -32768);
    // CreateInstGenerator(Prestate, holdVolEnv, -32768);
    // CreateInstGenerator(Prestate, decayVolEnv, -7973);
    // CreateInstGenerator(Prestate, sustainVolEnv, 0);
    // CreateInstGenerator(Prestate, releaseVolEnv, -7973);

    // genAmountType KeyRange = { .ranges = { 0, 0x7f } };
    // CreateInstGenerator(Prestate, keyRange, KeyRange);

    // genAmountType VelRange = { .ranges = { 0, 0x7f } };
    // CreateInstGenerator(Prestate, velRange, VelRange);

    genAmountType Pan = { .wAmount = 0 };
    CreateInstGenerator(Prestate, pan, Pan);

    genAmountType SampleIndex = { .wAmount = 0 };
    CreateInstGenerator(Prestate, sampleID, SampleIndex);

    // CreateInstGenerator(Prestate, overridingRootKey, Params.Key);

    // CreateInstGenerator(Prestate, coarseTune, Params.TuningOffset);
    //CreateInstGenerator(Prestate, fineTune, Params.TuningOffset);

    return(InstBagCount);
}

internal void
CreateTerminals(sf_prestate *Prestate)
{
    Assert(Prestate->PresetCount < ArrayCount(Prestate->Presets));
    sfPresetHeader *Preset =
        Prestate->Presets + Prestate->PresetCount++;
    ZeroStruct(*Preset);
    sprintf(Preset->achPresetName, "%s", "EOP");
    // Preset->wPreset = 0xbfdb;
    // Preset->wBank = 0xff5f;
    Preset->wPresetBagNdx = Prestate->PresetBagCount;

    Assert(Prestate->PresetBagCount < ArrayCount(Prestate->PresetBags));
    sfPresetBag *PresetBag =
        Prestate->PresetBags + Prestate->PresetBagCount++;
    ZeroStruct(*PresetBag);
    PresetBag->wGenNdx = Prestate->PresetGeneratorCount;
    PresetBag->wModNdx = Prestate->PresetModulatorCount;

    Assert(Prestate->PresetModulatorCount < ArrayCount(Prestate->PresetModulators));
    sfModList *PresetModulator =
        Prestate->PresetModulators + Prestate->PresetModulatorCount++;
    ZeroStruct(*PresetModulator);

    Assert(Prestate->PresetGeneratorCount < ArrayCount(Prestate->PresetGenerators));
    sfGenList *PresetGenerator =
        Prestate->PresetGenerators + Prestate->PresetGeneratorCount++;
    ZeroStruct(*PresetGenerator);

    Assert(Prestate->InstrumentCount < ArrayCount(Prestate->Instruments));
    sfInst *Instrument =
        Prestate->Instruments + Prestate->InstrumentCount++;
    ZeroStruct(*Instrument);
    sprintf(Instrument->achInstName, "%s", "EOI");
    Instrument->wInstBagNdx = Prestate->InstrumentBagCount;

    Assert(Prestate->InstrumentBagCount < ArrayCount(Prestate->InstrumentBags));
    sfInstBag *InstrumentBag =
        Prestate->InstrumentBags + Prestate->InstrumentBagCount++;
    ZeroStruct(*InstrumentBag);
    InstrumentBag->wInstGenNdx = Prestate->InstrumentGeneratorCount;
    InstrumentBag->wInstModNdx = Prestate->InstrumentModulatorCount;

    Assert(Prestate->InstrumentModulatorCount < ArrayCount(Prestate->InstrumentModulators));
    sfModList *InstrumentModulator =
        Prestate->InstrumentModulators + Prestate->InstrumentModulatorCount++;
    ZeroStruct(*InstrumentModulator);

    Assert(Prestate->InstrumentGeneratorCount < ArrayCount(Prestate->InstrumentGenerators));
    sfInstGenList *InstrumentGenerator =
        Prestate->InstrumentGenerators + Prestate->InstrumentGeneratorCount++;
    ZeroStruct(*InstrumentGenerator);

    Assert(Prestate->SampleCount < ArrayCount(Prestate->Samples));
    sfSample *Sample =
        Prestate->Samples + Prestate->SampleCount++;
    ZeroStruct(*Sample);
    sprintf(Sample->achSampleName, "%s", "EOS");

}


///////////////////////////////////////////////////////////////////////////////
// Main ///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if 0
internal size_t
WriteChunk(chunk *Chunk, FILE *File)
{
    fwrite(&Chunk->ckID, sizeof(Chunk->ckID), 1, File);
    fwrite(&Chunk->ckSize, sizeof(Chunk->ckSize), 1, File);
    fwrite(&Chunk->ckHeader, sizeof(Chunk->ckHeader), 1, File);

    size_t Result = sizeof(Chunk->ckID) + sizeof(Chunk->ckHeader) + sizeof(Chunk->ckSize);
    // TODO: (KAPSY) Would just be easier to iterate a sub chunk array here.

    return(Result);
}
#endif

internal size_t
WriteChunk(chunk *Chunk, FILE *File)
{
    fwrite(&Chunk->ckID, sizeof(Chunk->ckID), 1, File);
    fwrite(&Chunk->ckSize, sizeof(Chunk->ckSize), 1, File);
    fwrite(Chunk->ckDATA, Chunk->ckSize, 1, File);

    size_t Result = sizeof(Chunk->ckID) + sizeof(Chunk->ckSize) + Chunk->ckSize;
    return(Result);
}

internal size_t
WriteListChunk(chunk *Chunk, FILE *File)
{
    size_t SizeOfListChunkHeader = sizeof(FOURCC);

    fwrite(&Chunk->ckID, sizeof(Chunk->ckID), 1, File);
    fwrite(&Chunk->ckSize, sizeof(Chunk->ckSize), 1, File);
    fwrite(Chunk->ckDATA, SizeOfListChunkHeader, 1, File);

    size_t Result = sizeof(Chunk->ckID) + sizeof(Chunk->ckSize) + SizeOfListChunkHeader;
    return(Result);
}

// TODO: (KAPSY) The way we should be doing this is:
// - Everything is a chunk - no concept of chunks/sub_chunks
// - Chunks, the header is the data.
#if 0
internal chunk
NewChunk(char *ID, char *Header)
{
    chunk Result = {};
    Assert((StringLength(Header) == 4) && (StringLength(ID) == 4));
    Result.ckID = *((FOURCC *)ID);
    Result.ckHeader = *((FOURCC *)Header);

    // NOTE: (KAPSY) The header is included in the size.
    size_t SizeOfChunkHeader = sizeof(FOURCC);
    Result.ckSize = SizeOfChunkHeader;

    return(Result);
}
#endif

internal chunk *
NewChunk(chunk *Parent, char *ID, size_t Size, void *Data)
{
    Assert(Parent);
    Assert((StringLength(ID) == 4));

    if(!Parent->Children)
    {
        Parent->Children = ChunkPool + ChunkPoolPosition;
        ChunkPoolPosition += MAX_CHILDREN_CHUNKS;
    }


    Assert(Parent->ChildrenCount < MAX_CHILDREN_CHUNKS);

    chunk *Result = Parent->Children + Parent->ChildrenCount++;
    Result->ckID = *((FOURCC *)ID);
    Result->ckSize = Size;
    Result->ckDATA = (BYTE *)Data;

    return(Result);
}

internal chunk *
NewStringChunk(chunk *ParentChunk, char *ID, char *String)
{
    chunk *Result = NewChunk(ParentChunk, ID, StringLength(String), String);
    return(Result);
}

internal chunk
NewRIFFChunk()
{
    chunk Result = {};
    Result.ckID = *((FOURCC *)"RIFF");
    Result.ckSize = sizeof(FOURCC);
    Result.ckDATA = (BYTE *)"sfbk";
    return(Result);
}

internal char *
WordAlignedString(char *String, int *OutLength)
{
    char *Result = 0;
    int Length = StringLength(String) + 1;
    if(Length%2 != 0)
    {
        ++Length;
    }

    Result = (char *)calloc(Length, 0);
    sprintf(Result, "%s", String);

    *OutLength = Length;
    return(Result);
}

inline size_t
TotalSize(chunk *Chunk)
{
    size_t Result = 0;
    Result+= sizeof(Chunk->ckID);
    Result+= sizeof(Chunk->ckSize);
    Result+= Chunk->ckSize;
    return(Result);
}


internal size_t
CalcChunkSizes(chunk *Chunk)
{
    for(u32 ChildIndex = 0;
            ChildIndex < Chunk->ChildrenCount;
            ++ChildIndex)
    {
        chunk *Child = Chunk->Children + ChildIndex;
        Chunk->ckSize += CalcChunkSizes(Child);
    }

    return(TotalSize(Chunk));
}

internal void
WriteAllChunks(chunk *Root, FILE *File, size_t *BytesWritten)
{
    if(Root->Children)
    { *BytesWritten+=WriteListChunk(Root, File); }
    else
    { *BytesWritten+=WriteChunk(Root, File); }

    for(u32 ChildIndex = 0;
            ChildIndex < Root->ChildrenCount;
            ++ChildIndex)
    {
        chunk *Child = Root->Children + ChildIndex;
        WriteAllChunks(Child, File, BytesWritten);
    }
}


int main(int argc, char **argv)
{
    // TODO: (KAPSY)
    // So here's what we want to do:
    // -Load a wav file into memory.
    // -Specify the input params (divisions csvs, mono one channel,
    // normalize...)
    //
    // Example: sf2bb --input my_cool_beat.wav --divisions 1,1,1,2,2,2,1,1,3
    // --monoleft --normalize
    //
    // -Create the SF2 file, will probably have to define the header, arrays
    // that point to each division, and the division data, and then write it
    // out appropriately.
    // Would be great to be able to batch - would actually rather do that in C
    // here than in a script.

    kapsy_PrintArgs(argc, argv);

    collated_args CollatedArgs = {};

    kapsy_GetCollatedArgs(argc, argv, ArgDefs, ArrayCount(ArgDefs), &CollatedArgs);

    kapsy_PrintCollatedArgs(&CollatedArgs);

    if(CollatedArgs.Args[sf2bbArg_Input].ValueCount)
    {
        Assert(CollatedArgs.Args[sf2bbArg_Input].ValueCount == 1);

        char *WavPath = CollatedArgs.Args[sf2bbArg_Input].Values[0];
#if 0
        unsigned int WavSize = GetWaveFileSize(WavPath);


        wav_header WavHeader = LoadWavFile(WavPath);

        char *WavData = ;
#endif

        // NOTE: (KAPSY) Setup the SF pre state.
        sf_prestate Prestate = {};

        // int SampleIndex = CreateSample(&Prestate, CollatedArgs.Args[sf2bbArg_Input].Values[0]);
        // int SampleIndex = CreateSample(&Prestate, "/Users/pitorimaikeru/audio/fukth34mat_japan/_apps/sf2bb/data/beat_byte_daiei_031.wav");

        int SampleIndex = CreateTestSample(&Prestate);

        sf_inst_params InstParams =
        {
            .Name = "MyInstrument"
        };

        int InstrumentIndex = CreateInst(&Prestate, InstParams);
        // TODO: (KAPSY) Should be part of the instrument?
        int CurrentNote = C3;
        int TuningOffset = 6;

        sf_preset_params PresetParams =
        {
            .Name = "MyPreset"
        };

        int PresetIndex = CreatePreset(&Prestate, PresetParams);

        int Divisions[] = { 2, 2, 2, 2, 2, 2, 2, 2 };
        int DivisionsStart = 0;

        // NOTE: (KAPSY) Get the division values in frames.
        int TotalDivisions = 0;
        for(int DivIndex=0;
                DivIndex < ArrayCount(Divisions);
                ++DivIndex)
        {
            TotalDivisions+=Divisions[DivIndex];
        }

        float FrameRate = 44100.f; // TODO: (KAPSY) Take this from the sample.
        float DivisionSize = 16.f; // TODO: (KAPSY) Parameterize?
        float BeatsPerMinute = 130.f;
        float MeasuresPerMinute = BeatsPerMinute/4.f;
        float SecondsPerMeasure = 1.f/(MeasuresPerMinute/60.f);
        float FramesPerDivision = (SecondsPerMeasure/DivisionSize)*FrameRate;
        float FramePos = (float)DivisionsStart*FramesPerDivision;

        CreateGlobalZone(&Prestate);

        for(int DivIndex=0;
                DivIndex < 1;//ArrayCount(Divisions);
                ++DivIndex)
        {
            float FramesThisDivision =
                (float)Divisions[DivIndex]*FramesPerDivision;

            sf_zone_params ZoneParams =
            {
                .InstrumentIndex = InstrumentIndex,
                .SampleIndex = SampleIndex,
                .SampleStart = RoundReal32ToUInt32(FramePos),
                .SampleEnd = RoundReal32ToUInt32(FramePos + FramesThisDivision),
                .Key = CurrentNote,
                .TuningOffset = TuningOffset,
            };
            CreateZone(&Prestate, ZoneParams);

            FramePos+=FramesThisDivision;
            ++CurrentNote;
        }

        CreateTerminals(&Prestate);

        FILE *OutputSF2 = fopen("testout.sf2", "wd");

        chunk RiffChunk = NewRIFFChunk();

        ///////////////////////////////////////////////////////////////////////
        // Info Chunk /////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////

        chunk *InfoList = NewStringChunk(&RiffChunk, "LIST", "INFO");

        sfVersionTag VersionTag = { 1, 0 };
        NewChunk(InfoList, "ifil",
                sizeof(sfVersionTag), (void *)&VersionTag);

        int EngineStringLength = 0;
        char *EngineString = WordAlignedString("EMU8000", &EngineStringLength);
        NewChunk(InfoList, "isng",
                EngineStringLength, (void *)EngineString);

        int NameStringLength = 0;
        char *NameString = WordAlignedString("Test", &NameStringLength);
        NewChunk(InfoList, "INAM",
                NameStringLength, (void *)NameString);

        int CreatorStringLength = 0;
        char *CreatorString = WordAlignedString("Polyphone", &CreatorStringLength);
        NewChunk(InfoList, "ISFT",
                CreatorStringLength, (void *)CreatorString);
        ///////////////////////////////////////////////////////////////////////
        // sdta Chunk /////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////

        chunk *sdtaList = NewStringChunk(&RiffChunk, "LIST", "sdta");

        u32 BytesPerWord = 2;
        u32 SampleData16Size = Prestate.SampleDataCount*BytesPerWord;
        NewChunk(sdtaList, "smpl",
                SampleData16Size, (void *)Prestate.SampleData16);

        // u32 SampleData24Size = Prestate.SampleDataCount;
        // NewChunk(sdtaList, "sm24",
                // SampleData24Size, (void *)Prestate.SampleData24);

        ///////////////////////////////////////////////////////////////////////
        // pdta Chunk /////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////

        chunk *pdtaList = NewStringChunk(&RiffChunk, "LIST", "pdta");

        NewChunk(pdtaList, "phdr",
                sizeof(sfPresetHeader)*Prestate.PresetCount,
                (void *)Prestate.Presets);

        NewChunk(pdtaList, "pbag",
                sizeof(sfPresetBag)*Prestate.PresetBagCount,
                (void *)Prestate.PresetBags);

        NewChunk(pdtaList, "pmod",
                sizeof(sfModList)*Prestate.PresetModulatorCount,
                (void *)Prestate.PresetModulators);

        NewChunk(pdtaList, "pgen",
                sizeof(sfGenList)*Prestate.PresetGeneratorCount,
                (void *)Prestate.PresetGenerators);

        NewChunk(pdtaList, "inst",
                sizeof(sfInst)*Prestate.InstrumentCount,
                (void *)Prestate.Instruments);

        NewChunk(pdtaList, "ibag",
                sizeof(sfInstBag)*Prestate.InstrumentBagCount,
                (void *)Prestate.InstrumentBags);

        NewChunk(pdtaList, "imod",
                sizeof(sfModList)*Prestate.InstrumentModulatorCount,
                (void *)Prestate.InstrumentModulators);

        NewChunk(pdtaList, "igen",
                sizeof(sfInstGenList)*Prestate.InstrumentGeneratorCount,
                (void *)Prestate.InstrumentGenerators);

        NewChunk(pdtaList, "shdr",
                sizeof(sfSample)*Prestate.SampleCount,
                (void *)Prestate.Samples);


        CalcChunkSizes(&RiffChunk);

        size_t BytesWritten = 0;

        WriteAllChunks(&RiffChunk, OutputSF2, &BytesWritten);





#if 0
        size_t SizeOfChunkHeader = sizeof(FOURCC);
        size_t SizeOfChunkID = sizeof(FOURCC);
        size_t SizeOfChunkSize = sizeof(DWORD);

        size_t TotalWritten = 0;
        TotalWritten+=WriteChunk(&RiffChunk, OutputSF2);

        TotalWritten+=WriteChunk(&InfoList, OutputSF2);
        TotalWritten+=WriteChunk(&VersionChunk, OutputSF2);
        TotalWritten+=WriteChunk(&EngineChunk, OutputSF2);
        TotalWritten+=WriteChunk(&NameChunk, OutputSF2);

        TotalWritten+=WriteChunk(&sdtaList, OutputSF2);
        TotalWritten+=WriteChunk(&Sample16Chunk, OutputSF2);
        TotalWritten+=WriteChunk(&Sample24Chunk, OutputSF2);

        // TODO: (KAPSY) When adding the sub chunks should just add them to an
        // array within the main chunk and then iterate.
        TotalWritten+=WriteChunk(&pdtaList, OutputSF2);
        TotalWritten+=WriteChunk(&PresetHeadersChunk, OutputSF2);
        TotalWritten+=WriteChunk(&PresetBagsChunk, OutputSF2);
        TotalWritten+=WriteChunk(&PresetModulatorsChunk, OutputSF2);
        TotalWritten+=WriteChunk(&PresetGeneratorsChunk, OutputSF2);
        TotalWritten+=WriteChunk(&InstrumentsChunk, OutputSF2);
        TotalWritten+=WriteChunk(&InstrumentBagsChunk, OutputSF2);
        TotalWritten+=WriteChunk(&InstrumentModulatorsChunk, OutputSF2);
        TotalWritten+=WriteChunk(&InstrumentGeneratorsChunk, OutputSF2);
        TotalWritten+=WriteChunk(&SamplesChunk, OutputSF2);

        RiffChunk.ckSize += (TotalSize(&InfoList) +
                             TotalSize(&sdtaList) +
                             TotalSize(&pdtaList));
#endif

        size_t SizeOfChunkHeader = sizeof(FOURCC);
        size_t SizeOfChunkID = sizeof(FOURCC);
        size_t SizeOfChunkSize = sizeof(DWORD);

        Assert(BytesWritten ==
                (RiffChunk.ckSize + SizeOfChunkID + SizeOfChunkSize));

    }
    else
    {
        fprintf(stderr, "No input wave file specified!\n");
        return(EXIT_FAILURE);
    }

    printf("File successfully created!\n");

    return(EXIT_SUCCESS);
}
