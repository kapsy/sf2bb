#import<stdio.h>

int main(int argc, char **argv)
{
    FILE *OutFile = fopen("code/midi2note.h", "w");

    int CurrentMidiNumber = 0;
    int CurrentOctave = 0;

    fprintf(OutFile, "enum\n{\n");

    while(CurrentMidiNumber < (1 << 7))
    {
        if(0) {}
        else if(CurrentMidiNumber%12 == 0)
        {
            fprintf(OutFile, "    C%d  = %d,\n",
                    CurrentOctave, CurrentMidiNumber++);
        }
        else if(CurrentMidiNumber%12 == 1)
        {
            fprintf(OutFile, "    Cs%d = %d,\n",
                    CurrentOctave, CurrentMidiNumber++);
        }
        else if(CurrentMidiNumber%12 == 2)
        {
            fprintf(OutFile, "    D%d  = %d,\n",
                    CurrentOctave, CurrentMidiNumber++);
        }
        else if(CurrentMidiNumber%12 == 3)
        {
            fprintf(OutFile, "    Ds%d = %d,\n",
                    CurrentOctave, CurrentMidiNumber++);
        }
        else if(CurrentMidiNumber%12 == 4)
        {
            fprintf(OutFile, "    E%d  = %d,\n",
                    CurrentOctave, CurrentMidiNumber++);
        }
        else if(CurrentMidiNumber%12 == 5)
        {
            fprintf(OutFile, "    F%d  = %d,\n",
                    CurrentOctave, CurrentMidiNumber++);
        }
        else if(CurrentMidiNumber%12 == 6)
        {
            fprintf(OutFile, "    Fs%d = %d,\n",
                    CurrentOctave, CurrentMidiNumber++);
        }
        else if(CurrentMidiNumber%12 == 7)
        {
            fprintf(OutFile, "    G%d  = %d,\n",
                    CurrentOctave, CurrentMidiNumber++);
        }
        else if(CurrentMidiNumber%12 == 8)
        {
            fprintf(OutFile, "    Gs%d = %d,\n",
                    CurrentOctave, CurrentMidiNumber++);
        }
        else if(CurrentMidiNumber%12 == 9)
        {
            fprintf(OutFile, "    A%d  = %d,\n",
                    CurrentOctave, CurrentMidiNumber++);
        }
        else if(CurrentMidiNumber%12 == 10)
        {
            fprintf(OutFile, "    As%d = %d,\n",
                    CurrentOctave, CurrentMidiNumber++);
        }
        else if(CurrentMidiNumber%12 == 11)
        {
            fprintf(OutFile, "    B%d  = %d,\n",
                    CurrentOctave, CurrentMidiNumber++);
            fprintf(OutFile, "\n");
            CurrentOctave++;
        }
    }
    fprintf(OutFile, "};");


    printf("midi2note Preprocess successful!\n");
    return(0);
};
