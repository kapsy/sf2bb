#include <stdio.h>
#include <stdlib.h>
#include "kapsy.h"
#include "midi2note.h"

///////////////////////////////////////////////////////////////////////////////
// Arg Handling ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
enum
{
    sf2bbArg_Input,
    sf2bbArg_BPM,
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

#define MAX_ARG_INFO_VALUES 16

struct collated_arg
{
    int DefIndex;
    char *Values[MAX_ARG_INFO_VALUES];
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
            ArgIndex < Args->Count;
            ++ArgIndex)
    {
        collated_arg *Arg = Args->Args + ArgIndex;
        printf("ArgIndex: %d\n", ArgIndex);
        printf("DefIndex: %d\n", Arg->DefIndex);
        for(int ValueIndex = 0;
                ValueIndex < Arg->ValueCount;
                ++ValueIndex)
        {
            printf("Value: %s\n", Arg->Values[ValueIndex]);
        }
        printf("\n");
    }
}

collated_args
kapsy_GetCollatedArgs(int ArgCount, char **Args, char **Defs, int DefsCount)
{
    collated_args Result = {};
    collated_arg *CurrentArg = 0;

    for(int ArgIndex = 1;
            ArgIndex < ArgCount;
            ++ArgIndex)
    {
        char *ArgRaw = *(Args + ArgIndex);
        int DefFound = 0;

        for(int DefIndex = 0;
                DefIndex < DefsCount;
                ++DefIndex)
        {
            char *Def = *(Defs + DefIndex);
            if(kapsy_StringComp(Def, ArgRaw))
            {
                CurrentArg = Result.Args + Result.Count++;
                CurrentArg->DefIndex = DefIndex;
                DefFound = 1;
                break;
            }
        }

        if(!DefFound && CurrentArg)
        {
            char **CurrentArgValue = (CurrentArg->Values + CurrentArg->ValueCount++);
            *CurrentArgValue = ArgRaw;
        }
    }

    return(Result);

}

///////////////////////////////////////////////////////////////////////////////
// SF2 Spec Structs ///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// NOTE: (KAPSY) Four Character Code
typedef DWORD FOURCC;

struct sub_chunk
{
    FOURCC ckID;
    DWORD ckSize;
    BYTE *ckDATA;
};

struct chunk
{
    FOURCC ckHeader;
    FOURCC ckID;
    DWORD ckSize;
};

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

typedef short SHORT;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned char BYTE;

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

// TODO: (KAPSY) Should just make these enums.
typedef WORD SFGenerator;
typedef WORD SFModulator;

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

enum SFSampleLink
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

struct sf_sample_params
{
    char *Path;
};

struct sf_inst_params
{
};


struct sf_zone_params
{
    int SampleIndex;
    int SampleStart;
    int SampleEnd;
    int Key;
    float TuningOffset;
};

// NOTE: (KAPSY) All Create() functions are directly translated to the
// appropriate SF2 structs for file writing. For a more complete editor we
// would store everything in an editable state.
struct sf_pre_state
{
    // TODO: (Kapsy) Probably don't need these arrays if we're already storing
    // everything in the format most appropriate to writing out.
    // sf2_sample Samples[MAX_SAMPLES];
    // int SampleCount;
    // sf2_inst Instruments[SF2_MAX_INSTRUMENTS];
    // int InstrumentCount;
    // sf2_zone Zones[SF2_MAX_ZONES];
    // int ZoneCount;

    // TODO: (Kapsy) Need to add the following lists of data for SF2 files: (all lists)
    sfPresetHeader PresetHeaders[MAX_PRESETS];
    int PresetHeaderCount;

    sfPresetBag PresetBags[MAX_PRESETS];
    int PresetBagCount;

    sfModList PresetModulators[MAX_PRESETS];
    int PresetModulatorCount;

    sfGenList PresetGenerators[MAX_PRESETS];
    int PresetGeneratorCount;

    sfInst Instruments[MAX_INSTRUMENTS];
    int InstrumentCount;

    sfInstBag InstrumentBags[MAX_INSTRUMENTS];
    int InstrumentBagCount;

    sfModList InstrumentModulators[MAX_ZONE_MODULATORS];
    int InstrumentModulatorCount;

    sfInstGenList InstrumentGenerators[MAX_ZONE_GENERATORS];
    int InstrumentGeneratorCount;

    sfSample Samples[MAX_SAMPLES];
    int SampleCount;
;

internal int
CreateSample(sf_pre_state *PreState, char *Path)
{
    Assert(PreState->SampleCount < ArrayCount(PreState->Samples));

    int Index = PreState->SampleCount++;
    sfSample *Sample = PreState->Samples + Index;

    // TODO: (KAPSY) Do we load the sample directly here? Would seem to make
    // sense, but might be better to just refer to the file and load when
    // called upon?
    //
    u32 SampleCount = ;

    u32 SampleStart = PreState->SampleDataCount;
    u32 SampleEnd = SampleStart + SampleCount;
    PreState->SampleDataCount = SampleEnd;

    // TODO: (KAPSY) Not sure if this is the way to go - do we have to create a
    // new "sample" for each chop? Or, is that the role of a zone?
    Sample->achSampleName[20] = 0;
    sprintf(Sample->achSampleName[20]...);
    Sample->dwStart = SampleStart; // Offset from the sample data start.
    Sample->dwEnd = SampleEnd;
    Sample->dwStartloop = 0;
    Sample->dwEndloop = 0;
    Sample->dwSampleRate = SampleRate;
    Sample->byOriginalPitch = 255;
    Sample->chPitchCorrection = 0;
    Sample->wSampleLink = ; // TODO: (KAPSY) Index for the sample of the other channel - interleaved not supported!
    Sample->sfSampleType = leftSample;

    return(Index);
}

internal int
CreateInst(sf_pre_state *PreState, sf_inst_params Params)
{
    Assert(PreState->InstrumentCount < ArrayCount(PreState->Instruments));
    int Index = PreState->InstrumentCount++;
    sfInst *Inst = PreState->Instruments + Index;

    Assert(StringLength(Params->Name) < ArrayLength(Inst->achInstName));
    sprintf(&Inst->achInstName, Params->Name);

    // NOTE: (KAPSY) Index to the instruments zone list.
    Inst->wInstBagNdx = PreState->InstrumentBagCount;


#if 0
    for(int ZoneIndex = 0;
            ZoneIndex < Params.ZoneCount;
            ++ZoneIndex)
    {
        sf_zone_params *ZoneParams = InstParams + ZoneIndex;

        Assert(PreState->InstrumentBagCount < ArrayCount(PreState->InstrumentBags));
        sfInstBag *InstBag = PreState->InstrumentBags + PreState->InstrumentBagCount++;

        // TODO: (KAPSY) Set up the 


    }
#endif






    return(Index);
}

internal
CreateInstGenerator(sf_prestate *PreState, int GenOper, int GenAmount)
{
    Assert((PreState->InstrumentGeneratorCount + GeneratorsPerZone) <
            ArrayCount(PreState->InstrumentGenerators));

    sfInstGenList *Generator = PreState->InstrumentGenerators +
        PreState->InstrumentGeneratorsCount++;

    Generator->sfGenOper = GenOper;
    Generator->genAmount = GenAmount;
}

// TODO: (KAPSY) Are we even able to provide an index here?
internal int
CreateZone(sf_pre_state *PreState, sf_zone_params Params)
{
    int InstrumentIndex = Params.InstrumentIndex;
    sfInst *Inst = PreState->Instruments + InstrumentIndex;

    // TODO: (KAPSY) So these are zones, and we have to create at least as many as we have hits for.
    Assert(PreState->InstrumentBagCount < ArrayCount(PreState->InstrumentBags));
    sfInstBag *InstBag = PreState->InstrumentBags + PreState->InstrumentBagCount++;

    InstBag->wInstGenNdx = PreState->InstrumentGeneratorCount;
    InstBag->wInstModNdx = PreState->InstrumentModulatorCount;

    CreateInstGenerator(PreState, startAddrsOffset, Params.SampleStart);
    CreateInstGenerator(PreState, endAddrsOffset, Params.SampleEnd);

    CreateInstGenerator(PreState, delayVolEnv, Params.);
    CreateInstGenerator(PreState, attackVolEnv, Params.);
    CreateInstGenerator(PreState, holdVolEnv, Params.);
    CreateInstGenerator(PreState, decayVolEnv, Params.);
    CreateInstGenerator(PreState, sustainVolEnv, Params.);
    CreateInstGenerator(PreState, releaseVolEnv, Params.);

    CreateInstGenerator(PreState, keyRange, Params.Key);
    CreateInstGenerator(PreState, velRange, ??);

    CreateInstGenerator(PreState, sampleID, SampleIndex);
    CreateInstGenerator(PreState, overridingRootKey, Params.Key);

    CreateInstGenerator(PreState, courseTune, Params.TuningOffset);
    CreateInstGenerator(PreState, fineTune, Params.TuningOffset);



    return(Result);
}

internal
CreateTerminals(sf_pre_state *PreState)
{
    Assert(PreState->PresetHeaderCount < ArrayCount(PreState->PresetHeaders));
    sfPresetHeader *PresetHeader =
        PreState->PresetHeaders + PreState->PresetHeaderCount++;
    ZeroStruct(*PresetHeader);

    Assert(PreState->PresetBagCount < ArrayCount(PreState->PresetBags));
    sfPresetBag *PresetBag =
        PreState->PresetBags + PreState->PresetBagCount++;
    ZeroStruct(*PresetBag);

    Assert(PreState->PresetModulatorCount < ArrayCount(PreState->PresetModulators));
    sfModList *PresetModulator =
        PreState->PresetModulators + PreState->PresetModulatorCount++;
    ZeroStruct(*PresetModulator);

    Assert(PreState->PresetGeneratorCount < ArrayCount(PreState->PresetGenerators));
    sfGenList *PresetGenerator =
        PreState->PresetGenerators + PreState->PresetGeneratorCount++;
    ZeroStruct(*PresetGenerator);

    Assert(PreState->InstrumentCount < ArrayCount(PreState->Instruments));
    sfInst *Instrument =
        PreState->Instruments + PreState->InstrumentCount++;
    ZeroStruct(*Instrument);

    Assert(PreState->InstrumentBagCount < ArrayCount(PreState->InstrumentBags));
    sfInstBag *InstrumentBag =
        PreState->InstrumentBags + PreState->InstrumentBagCount++;
    ZeroStruct(*InstrumentBag);

    Assert(PreState->InstrumentModulatorCount < ArrayCount(PreState->InstrumentModulators));
    sfModList *InstrumentModulator =
        PreState->InstrumentModulators + PreState->InstrumentModulatorCount++;
    ZeroStruct(*InstrumentModulator);

    Assert(PreState->InstrumentGeneratorCount < ArrayCount(PreState->InstrumentGenerators));
    sfInstGenList *InstrumentGenerator =
        PreState->InstrumentGenerators + PreState->InstrumentGeneratorCount++;
    ZeroStruct(*InstrumentGenerator);

    Assert(PreState->SampleCount < ArrayCount(PreState->Samples));
    sfSample *Sample =
        PreState->Samples + PreState->SampleCount++;
    ZeroStruct(*Sample);
}


///////////////////////////////////////////////////////////////////////////////
// Main ///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

inline void
WriteSubChunk(sub_chunk *SubChunk, FILE *File)
{
    fwrite(&SubChunk->ID, StringLength(SubChunk->ID), 1, OutputSF2);
    fwrite(&SubChunk->Size, sizeof(SubChunk->Size), 1, OutputSF2);
    fwrite(SubChunk->Data, SubChunk->Size, 1, OutputSF2);
}

internal chunk
NewChunk(char *ID, char *Header, size_t Size = 0)
{
    chunk Result = {};
    Assert((StringLength(Header) == 4) && (StringLength(ID) == 4));
    Result.ckID = (FOURCC)(*ID);
    Result.ckHeader = (FOURCC)(*Header);
    Result.ckSize = Size;

    return(Result);
}

internal sub_chunk
NewSubChunk(chunk *ParentChunk, char *ID, size_t Size, void *Data)
{
    chunk Result = {};
    Assert((StringLength(ID) == 4));
    Result.ckID = (FOURCC)(*ID);
    Result.ckSize = Size;
    Result.ckData = (BYTE *)Data;
    ParentChunk->Size += Size;

    return(Result);
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

    collated_args CollatedArgs[ArrayCount(ArgDefs)] = {};

    kapsy_GetCollatedArgs(argc, argv, ArgDefs, ArrayCount(ArgDefs), &CollatedArgs);

    kapsy_PrintCollatedArgs(&CollatedArgs);

    if(CollatedArgs[sf2bbArg_Input].Found)
    {
        Assert(CollatedArgs[sf2bbArg_Input].Count == 1);

        char *WavPath = [sf2bbArg_Input].Values[0];
        unsigned int WavSize = GetWaveFileSize(WavPath);


        wav_header WavHeader = LoadWavFile(WavPath);

        char *WavData = ;

        // NOTE: (KAPSY) Setup the SF pre state.
        sf_pre_state PreState = {};

        int SampleIndex = CreateSample(&PreState, [sf2bbArg_Input].Values[0]);

        int InstrumentIndex = CreateInstrument(&PreState, InstrumentParams);
        // TODO: (KAPSY) Should be part of the instrument?
        int CurrentNote = C3;
        int TuningOffset = 6;

        // NOTE: (KAPSY) Get the division values in frames.
        int TotalDivisions = 0;
        for(int DivIndex=0;
                DivIndex < ArrayCount(Divisions);
                ++DivIndex)
        {
            TotalDivisions+=Divisions[DivIndex];
        }

        float FrameRate = 44100.f;
        float DivisionSize = 16.f; // TODO: (KAPSY) Parameterize?
        float BeatsPerMinute = ;
        float MeasuresPerMinute = BeatsPerMinute/4.f;
        float SecondsPerMeasure = 1.f/(MeasuresPerMinute/60.f);
        float FramesPerDivision = (SecondsPerMeasure/DivisionSize)*FrameRate;
        float FramePos = (float)DivisionsStart*FramesPerDivision;

        for(int DivIndex=0;
                DivIndex < ArrayCount(Divisions);
                ++DivIndex)
        {
            float FramesThisDivision =
                (float)Divisions[DivIndex]*FramesPerDivision;

            sf_zone_params ZoneParams =
            {
                .SampleIndex = SampleIndex,
                .SampleStart = RoundReal32ToUInt32(FramePos),
                .SampleEnd = RoundReal32ToUInt32(FramePos + FramesThisDivision),
                .Key = CurrentNote,
                .TuningOffset = TuningOffset,
            };
            CreateZone(InstrumentIndex, ZoneParams);

            FramePos+=FramesThisDivision;
        }

        CreateTerminals(&PreState);

        FILE *OutputSF2 = fopen(InputFileName, "wd");
        size_t BytesWritten = 0;

        chunk RiffChunk = NewChunk("RIFF", "sfbk");

        int ChunkSize = sizeof(sf2_chunk);

        ///////////////////////////////////////////////////////////////////////
        // Info Chunk /////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////

        chunk InfoList = NewChunk("LIST", "INFO");

        sfVersionTag VersionTag = { 1, 0 };
        sub_chunk VersionChunk = NewSubChunk(&InfoList, "ifil",
                sizeof(sfVersionTag), (void *)VersionTag);

        char *EngineString = WordAlignedString("Battery");
        sub_chunk EngineChunk = NewSubChunk(&InfoList, "isng",
                StringLength(EngineChunk), (void *)EngineString);

        char *NameString = WordAlignedString("Kapsy");
        sub_chunk NameChunk = NewSubChunk(&InfoList, "INAM",
                StringLength(NameString), (void *)NameString);

        ///////////////////////////////////////////////////////////////////////
        // sdta Chunk /////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////

        chunk sdtaList = NewChunk("LIST", "sdta");

        ///////////////////////////////////////////////////////////////////////
        // pdta Chunk /////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////

        chunk pdtaList = NewChunk("LIST", "pdta");


#if 0

            sfPresetHeader PresetHeaders[2] =
            {
                // {
                    // .achPresetName = "TestName";
                    // .wPreset = 0;
                    // .wBank = 0;
                    // .wPresetBagNdx = 0;
                    // .dwLibrary = 0;
                    // .dwGenre = 0;
                    // .dwMorphology = 0;
                // },
                {
                    .achPresetName = "EOP";
                    .wPreset = 0;
                    .wBank = 0;
                    .wPresetBagNdx = 0;
                    .dwLibrary = 0;
                    .dwGenre = 0;
                    .dwMorphology = 0;
                },
            }
            sub_chunk PresetHeadersChunk = NewSubChunk(&pdtaList, "phdr");
            PresetHeadersChunk.Data = (void *)PresetHeaders;
            PresetHeadersChunk.Size = sizeof(PresetHeaders);

            // NOTE: (KAPSY) Seems that each bag needs a terminator???
            sfPresetBag PresetBags[] =
            {
                // {
                    // .wGenNdx = 0;
                    // .wModNdx = 0;
                // },
                // TODO: (Kapsy) Terminal Zone. Does this mean that all indexes
                // start from one?
                // Looking at the example file this doesn't seem to be the
                // case.
                {
                    .wGenNdx = 0;
                    .wModNdx = 0;
                }
            };
            sub_chunk PresetBagChunk = NewSubChunkA(&pdtaList, "pbag");
            PresetBagChunk.Data = (void *)PresetBags;
            PresetBagChunk.Size = sizeof(PresetBags);

            // NOTE: (KAPSY) Since we don't use these we just use a terminator.
            sfModList PresetModulators[] =
            {
                {
                    sfModSrcOper = 0;
                    sfModDestOper = 0;
                    modAmount = 0;
                    sfModAmtSrcOper = 0;
                    sfModTransOper = 0;
                }
            };
            sub_chunk PresetModulatorsChunk = NewSubChunk(&pdtaList, "pmod");
            PresetModulatorsChunk.Data = (void *)PresetModulators;
            PresetModulatorsChunk.Size = sizeof(PresetModulators);


            sfGenList PresetGenerators[] =
            {
                // {
                    // .sfGenOper = 0;
                    // .genAmount = 0;
                // },
                {
                    .sfGenOper = 0;
                    .genAmount = 0;
                },
            };
            sub_chunk PresetGeneratorsChunk = NewSubChunk(&pdtaList, "pgen");
            PresetGeneratorsChunk.Data = (void *)PresetGenerators;
            PresetGeneratorsChunk.Size = sizeof(PresetGenerators);
#endif



            sub_chunk PresetHeadersChunk =
                NewSubChunk(&pdtaList, "phdr",
                        sizeof(sfPresetHeader)*PreState.PresetHeaderCount,
                        (void *)PreState.PresetPresetHeaders);

            sub_chunk PresetBagsChunk =
                NewSubChunk(&pdtaList, "pbag",
                        sizeof(sfPresetBag)*PreState.PresetBagCount,
                        (void *)PreState.PresetPresetBags);

            sub_chunk PresetModulatorsChunk =
                NewSubChunk(&pdtaList, "pmod",
                        sizeof(sfModList)*PreState.PresetModulatorCount,
                        (void *)PreState.PresetPresetModulators);

            sub_chunk PresetGeneratorsChunk =
                NewSubChunk(&pdtaList, "pgen",
                        sizeof(sfGenList)*PreState.PresetGeneratorCount,
                        (void *)PreState.PresetPresetGenerators);

            sub_chunk InstrumentsChunk =
                NewSubChunk(&pdtaList, "inst",
                        sizeof(sfInst)*PreState.InstrumentCount,
                        (void *)PreState.Instruments);

            sub_chunk InstrumentBagsChunk =
                NewSubChunk(&pdtaList, "ibag",
                        sizeof(sfInstBag)*PreState.InstrumentBagCount,
                        (void *)PreState.InstrumentBags);

            sub_chunk InstrumentModulatorsChunk =
                NewSubChunk(&pdtaList, "imod",
                        sizeof(sfModList)*PreState.InstrumentModulatorCount,
                        (void *)PreState.InstrumentModulators);

            sub_chunk InstrumentGeneratorsChunk =
                NewSubChunk(&pdtaList, "igen",
                        sizeof(sfInstGenList)*PreState.InstrumentGeneratorCount,
                        (void *)PreState.InstrumentGenerators);

            sub_chunk SamplesChunk =
                NewSubChunk(&pdtaList, "inst",
                        sizeof(sfInst)*PreState.SampleCount,
                        (void *)PreState.Samples);


#if 0
            sfInst Instruments[] =
            {
                {
                    .achInstName = "Kapsy's Instrument",
                    0
                },
                {
                    0,
                    0
                }
            }
            sub_chunk InstrumentsChunk = NewSubChunk(&pdtaList, "inst");
            InstrumentsChunk.Data = (void *)Instruments;
            InstrumentsChunk.Size = sizeof(InstrumentsChunk);

            sfInstBag InstrumentBags[] =
            {
                {
                    0,
                    0,
                },
                {
                    0,
                    0,
                },
            };
            sub_chunk InstrumentBagsChunk = NewSubChunk(&pdtaList, "ibag");
            InstrumentBagsChunk.Data = (void *)Instruments;
            InstrumentBagsChunk.Size = sizeof(InstrumentBagsChunk);


            sfModList InstrumentZoneModulators[] =
            {
                // TODO: (KAPSY) SHOULD use enums here so we know what the hell
                // we are doing.
                {
                    sfModSrcOper = 0;
                    sfModDestOper = 0;
                    modAmount = 0;
                    sfModAmtSrcOper = 0;
                    sfModTransOper = 0;
                },
                {
                    sfModSrcOper = 0;
                    sfModDestOper = 0;
                    modAmount = 0;
                    sfModAmtSrcOper = 0;
                    sfModTransOper = 0;
                }
            };
            sub_chunk InstrumentZoneModulatorsChunk = NewSubChunk(&pdtaList, "imod");
            InstrumentZoneModulatorsChunk.Data = (void *)InstrumentZoneModulators;
            InstrumentZoneModulatorsChunk.Size = sizeof(InstrumentZoneModulators);


            sfInstGenList InstrumentZoneGenerators[] =
            {
                {
                    0,
                    0,
                },
                {
                    0,
                    0,
                },
            };
            sub_chunk InstrumentZoneGeneratorsChunk = NewSubChunk(&pdtaList, "imod");
            InstrumentZoneGeneratorsChunk.Data = (void *)InstrumentZoneGenerators;
            InstrumentZoneGeneratorsChunk.Size = sizeof(InstrumentZoneGenerators);







            // WriteChunk(&InfoList, OutputSF2);
            // WriteSubChunk(&VersionChunk, OutputSF2);
            // WriteSubChunk(&EngineChunk, OutputSF2);
            // WriteSubChunk(&NameChunk, OutputSF2);
#endif

        }


    }
    else
    {
        return(EXIT_FAILURE);
    }




    return(EXIT_SUCCESS);
}
