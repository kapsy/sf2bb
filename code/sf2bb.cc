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
    99cent = 99,
    sampleModes = 54,
    scaleTuning = 56,
    exclusiveClass = 57,
    overridingRootKey = 58,
};

///////////////////////////////////////////////////////////////////////////////
// Internal SF2 Defs //////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct sf2_sample
{
    char *Path;
};

struct sf2_inst
{
    int Index;
};

struct sf2_zone
{
    int SampleRef = Sample->Index;
    int SampleStart = 1234;
    int SampleEnd = 5678;
    int KeyStart = C2;
    int KeyEnd = C2;
    int RootKey = C2;
    int TuningOffset = 6; // NOTE: (KAPSY) In semis.
};

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

    sfModList PresetZoneModulators[MAX_PRESETS];
    int PresetZoneModulatorCount;

    sfGenList PresetZoneGenerators[MAX_PRESETS];
    int PresetZoneGeneratorCount;

    sfInst Instruments[MAX_INSTRUMENTS];
    int InstrumentCount;

    sfInstBag InstrumentBags[MAX_INSTRUMENTS];
    int InstrumentBagCount;

    sfModList InstrumentZoneModulators[MAX_ZONE_MODULATORS];
    int InstrumentZoneModulatorCount;

    sfInstGenList InstrumentZoneGenerators[MAX_ZONE_GENERATORS];
    int InstrumentZoneGeneratorCount;

    sfSample Samples[MAX_SAMPLES];
    int SampleCount;
};

// TODO: (KAPSY) For all of these create functions probably better to return
// the index.
internal sfSample *
CreateSample(sf_pre_state *PreState, char *Path)
{
    sfSample *Result = 0;
    Assert(PreState->SampleCount < ArrayCount(PreState->Samples));
    Result = PreState->Samples + PreState->SampleCount++;

    // TODO: (KAPSY) Do we load the sample directly here? Would seem to make
    // sense, but might be better to just refer to the file and load when
    // called upon?

    Result->achSampleName[20] = 0;
    sprintf(Result->achSampleName[20]...);
    Result->dwStart = 0; // Offset from the sample data start.
    Result->dwEnd = 0;
    Result->dwStartloop = 0;
    Result->dwEndloop = 0;
    Result->dwSampleRate = 0;
    Result->byOriginalPitch = 0;
    Result->chPitchCorrection = 0;
    Result->wSampleLink = 0;
    Result->sfSampleType = 0;

    return(Result);
}

internal sf2_instrument *
CreateInstrument(sf_pre_state *PreState, sf2_instrument_params Params)
{
    sf2_instrument *Result = 0;
    Assert(PreState->InstrumentCount < ArrayCount(PreState->Instruments));
    Result = PreState->Instruments + PreState->InstrumentCount++;
    // TODO: (KAPSY) Fill out the params here!
    return(Result);
}

internal sf2_zone *
CreateZone(sf2_def *Def, sf2_zone Params)
{
    sf2_zone *Result = 0;
    Assert(Def->ZoneCount < ArrayCount(Def->Zones));
    Result = Def->Zones + Def->ZoneCount++;

    Result->SampleRef = Params.SampleRef;
    Result->SampleStart = Params.SampleStart;
    Result->SampleEnd = Params.SampleEnd;
    Result->KeyStart = Params.KeyStart;
    Result->KeyEnd = Params.KeyEnd;
    Result->RootKey = Params.RootKey;
    Result->TuningOffset = Params.TuningOffset;

    return(Result);
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

internal sub_chunk
NewSubChunk(&InfoList, "isng");

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


        // NOTE: (KAPSY) Setup the SF2 instrument defs.

        sf2_def SF2Def = {}; // NOTE: (KAPSY) The entire patch.
        sf2_sample *SF2Sample = CreateSample(&SF2Def, [sf2bbArg_Input].Values[0]);
        sf2_inst Inst = CreateInstrument(&SF2Def, InstrumentParams);
        int = CurrentNote = C3;
        // TODO: (KAPSY) Should we make this an input param.
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

            sf2_zone ZoneParams =
            {
                .SampleRef = SF2Sample->Index,
                .SampleStart = RoundReal32ToUInt32(FramePos),
                .SampleEnd = RoundReal32ToUInt32(FramePos + FramesThisDivision),
                .Key = CurrentNote,
                .TuningOffset = TuningOffset,
            };
            CreateZone(&Inst, ZoneParams);

            FramePos+=FramesThisDivision;
        }


2017年 9月 1日 金曜日 22時49分04秒 JST
TODO: (KAPSY) So, all we need to do here, is to go through the SF2 def, and put it into the right format.
Could make things easier by creating the right groups etc when we add?
Yes, I think that's the way - create contiguous banks of all the necessary structs when we make our def, then calculate sizes and read them out...




        FILE *OutputSF2 = fopen(InputFileName, "wd");
        size_t BytesWritten = 0;

        int RiffSize = 0;
        chunk RiffChunk = NewChunk(0, "RIFF", "sfbk");

        int ChunkSize = sizeof(sf2_chunk);

        ///////////////////////////////////////////////////////////////////////
        // Info Chunk /////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////

        // TODO: (KAPSY) *Might* make sense to make the whole thing a tree?
        // If sub_chunks can have further sub_chunks...
        // Appears they don't. But would still be better to write everything out at the end.
        {
            chunk InfoList = NewChunk(&RiffChunk, "LIST", "INFO");

            sub_chunk VersionChunk = NewSubChunk(&InfoList, "ifil", sizeof(sfVersionTag));

            sf_version_tag VersionTag = { , };
            sub_chunk VersionChunk = NewSubChunk(&InfoList, "ifil");
            VersionChunk.Data = &VersionTag;
            VersionChunk.Size = sizeof(VersionTag);

            sub_chunk EngineChunk = NewSubChunk(&InfoList, "isng");
            EngineChunk.Data = WordAlignedString("Battery");
            EngineChunk.Size = StringLength(EngineChunk.Data);

            sub_chunk NameChunk = NewSubChunk(&InfoList, "INAM");
            NameChunk.Data = WordAlignedString("Kapsy");
            NameChunk.Size = StringLength((char *)NameChunk.Data);

            chunk pdtaList = NewChunk(&RiffChunk, "LIST", "pdta");


            sfPresetHeader PresetHeaders[2] =
            {
                {
                    .achPresetName = "TestName";
                    .wPreset = 0;
                    .wBank = 0;
                    .wPresetBagNdx = 0;
                    .dwLibrary = 0;
                    .dwGenre = 0;
                    .dwMorphology = 0;
                },
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
                {
                    .wGenNdx = 0;
                    .wModNdx = 0;
                },
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
            // TODO: (KAPSY) Pretty sure that these are preset zone modulators
            // - should name these appropriately.
            sfModList Modulators[] =
            {
                {
                    sfModSrcOper = 0;
                    sfModDestOper = 0;
                    modAmount = 0;
                    sfModAmtSrcOper = 0;
                    sfModTransOper = 0;
                }
            };
            sub_chunk ZoneModulatorsChunk = NewSubChunk(&pdtaList, "pmod");
            ZoneModulatorsChunk.Data = (void *)Modulators;
            ZoneModulatorsChunk.Size = sizeof(Modulators);


            sfGenList PresetGenerators[] =
            {
                {
                    .sfGenOper = 0;
                    .genAmount = 0;
                },
                {
                    .sfGenOper = 0;
                    .genAmount = 0;
                },
            };
            sub_chunk PresetGeneratorsChunk = NewSubChunk(&pdtaList, "pgen");
            PresetGeneratorsChunk.Data = (void *)PresetGenerators;
            PresetGeneratorsChunk.Size = sizeof(PresetGenerators);


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
        }

        ///////////////////////////////////////////////////////////////////////
        // sdta Chunk /////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////

        {
            sf2_chunk ListChunk = NewChunk("LIST", 0);
            sf2_chunk sdtaChunk = NewChunk("sdta", 0);
        }

        ///////////////////////////////////////////////////////////////////////
        // pdta Chunk /////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////

        {
            sf2_chunk ListChunk = NewChunk("LIST", 0);
            sf2_chunk pdtaChunk = NewChunk("pdta", 0);
        }

    }
    else
    {
        return(EXIT_FAILURE);
    }




    return(EXIT_SUCCESS);
}
