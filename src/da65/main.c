/*****************************************************************************/
/*                                                                           */
/*                                  main.c                                   */
/*                                                                           */
/*                  Main program for the da65 disassembler                   */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/* (C) 1998-2014, Ullrich von Bassewitz                                      */
/*                Roemerstrasse 52                                           */
/*                D-70794 Filderstadt                                        */
/* EMail:         uz@cc65.org                                                */
/*                                                                           */
/*                                                                           */
/* This software is provided 'as-is', without any expressed or implied       */
/* warranty.  In no event will the authors be held liable for any damages    */
/* arising from the use of this software.                                    */
/*                                                                           */
/* Permission is granted to anyone to use this software for any purpose,     */
/* including commercial applications, and to alter it and redistribute it    */
/* freely, subject to the following restrictions:                            */
/*                                                                           */
/* 1. The origin of this software must not be misrepresented; you must not   */
/*    claim that you wrote the original software. If you use this software   */
/*    in a product, an acknowledgment in the product documentation would be  */
/*    appreciated but is not required.                                       */
/* 2. Altered source versions must be plainly marked as such, and must not   */
/*    be misrepresented as being the original software.                      */
/* 3. This notice may not be removed or altered from any source              */
/*    distribution.                                                          */
/*                                                                           */
/*****************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>

/* common */
#include "abend.h"
#include "check.h"
#include "cmdline.h"
#include "cpu.h"
#include "debugflag.h"
#include "fname.h"
#include "print.h"
#include "version.h"

/* da65 */
#include "attrtab.h"
#include "code.h"
#include "comments.h"
#include "data.h"
#include "error.h"
#include "global.h"
#include "handler.h"
#include "infofile.h"
#include "labels.h"
#include "opctable.h"
#include "opc45GS02.h"
#include "output.h"
#include "scanner.h"
#include "segment.h"


static unsigned PrevAddrMode;


/*****************************************************************************/
/*                                   Code                                    */
/*****************************************************************************/



static void Usage (void)
/* Print usage information and exit */
{
    printf ("Usage: %s [options] [inputfile]\n"
            "Short options:\n"
            "  -d\t\t\tDebug mode\n"
            "  -g\t\t\tAdd debug info to object file\n"
            "  -h\t\t\tHelp (this text)\n"
            "  -i name\t\tSpecify an info file\n"
            "  -m\t\t\tRun multiple passes to resolve labels\n"
            "  -o name\t\tName the output file\n"
            "  -v\t\t\tIncrease verbosity\n"
            "  -F\t\t\tAdd formfeeds to the output\n"
            "  -S addr\t\tSet the start/load address\n"
            "  -s\t\t\tAccept line markers in the info file\n"
            "  -V\t\t\tPrint the disassembler version\n"
            "\n"
            "Long options:\n"
            "  --argument-column n\tSpecify argument start column\n"
            "  --comment-column n\tSpecify comment start column\n"
            "  --comments n\t\tSet the comment level for the output\n"
            "  --cpu type\t\tSet cpu type\n"
            "  --debug\t\tDebug mode\n"
            "  --debug-info\t\tAdd debug info to object file\n"
            "  --formfeeds\t\tAdd formfeeds to the output\n"
            "  --help\t\tHelp (this text)\n"
            "  --hexoffs\t\tUse hexadecimal label offsets\n"
            "  --info name\t\tSpecify an info file\n"
            "  --label-break n\tAdd newline if label exceeds length n\n"
            "  --mnemonic-column n\tSpecify mnemonic start column\n"
            "  --multi-pass\t\tRun multiple passes to resolve labels\n"
            "  --pagelength n\tSet the page length for the listing\n"
            "  --start-addr addr\tSet the start/load address\n"
            "  --sync-lines\t\tAccept line markers in the info file\n"
            "  --text-column n\tSpecify text start column\n"
            "  --verbose\t\tIncrease verbosity\n"
            "  --version\t\tPrint the disassembler version\n",
            ProgName);
}



static void RangeCheck (const char* Opt, unsigned long Val,
                        unsigned long Min, unsigned long Max)
/* Do a range check for the given option and abort if there's a range
** error.
*/
{
    if (Val < Min || Val > Max) {
        Error ("Argument for %s outside valid range (%ld-%ld)", Opt, Min, Max);
    }
}



static unsigned long CvtNumber (const char* Arg, const char* Number)
/* Convert a number from a string. Allow '$' and '0x' prefixes for hex
** numbers.
*/
{
    unsigned long Val;
    int           Converted;
    char          BoundsCheck;

    /* Convert */
    if (*Number == '$') {
        ++Number;
        Converted = sscanf (Number, "%lx%c", &Val, &BoundsCheck);
    } else {
        Converted = sscanf (Number, "%li%c", (long*)&Val, &BoundsCheck);
    }

    /* Check if we do really have a number */
    if (Converted != 1) {
        Error ("Invalid number given in argument: %s\n", Arg);
    }

    /* Return the result */
    return Val;
}



static void OptArgumentColumn (const char* Opt, const char* Arg)
/* Handle the --argument-column option */
{
    /* Convert the argument to a number */
    unsigned long Val = CvtNumber (Opt, Arg);

    /* Check for a valid range */
    RangeCheck (Opt, Val, MIN_ACOL, MAX_ACOL);

    /* Use the value */
    ACol = (unsigned char) Val;
}



static void OptBytesPerLine (const char* Opt, const char* Arg)
/* Handle the --bytes-per-line option */
{
    /* Convert the argument to a number */
    unsigned long Val = CvtNumber (Opt, Arg);

    /* Check for a valid range */
    RangeCheck (Opt, Val, MIN_BYTESPERLINE, MAX_BYTESPERLINE);

    /* Use the value */
    BytesPerLine = (unsigned char) Val;
}



static void OptCommentColumn (const char* Opt, const char* Arg)
/* Handle the --comment-column option */
{
    /* Convert the argument to a number */
    unsigned long Val = CvtNumber (Opt, Arg);

    /* Check for a valid range */
    RangeCheck (Opt, Val, MIN_CCOL, MAX_CCOL);

    /* Use the value */
    CCol = (unsigned char) Val;
}



static void OptComments (const char* Opt, const char* Arg)
/* Handle the --comments option */
{
    /* Convert the argument to a number */
    unsigned long Val = CvtNumber (Opt, Arg);

    /* Check for a valid range */
    RangeCheck (Opt, Val, MIN_COMMENTS, MAX_COMMENTS);

    /* Use the value */
    Comments = (unsigned char) Val;
}



static void OptCPU (const char* Opt attribute ((unused)), const char* Arg)
/* Handle the --cpu option */
{
    /* Find the CPU from the given name */
    CPU = FindCPU (Arg);
    SetOpcTable (CPU);
}



static void OptDebug (const char* Opt attribute ((unused)),
                      const char* Arg attribute ((unused)))
/* Disassembler debug mode */
{
    ++Debug;
}



static void OptDebugInfo (const char* Opt attribute ((unused)),
                          const char* Arg attribute ((unused)))
/* Add debug info to the object file */
{
    DebugInfo = 1;
}



static void OptFormFeeds (const char* Opt attribute ((unused)),
                          const char* Arg attribute ((unused)))
/* Add form feeds to the output */
{
    FormFeeds = 1;
}



static void OptHelp (const char* Opt attribute ((unused)),
                     const char* Arg attribute ((unused)))
/* Print usage information and exit */
{
    Usage ();
    exit (EXIT_SUCCESS);
}



static void OptHexOffs (const char* Opt attribute ((unused)),
                        const char* Arg attribute ((unused)))
/* Handle the --hexoffs option */
{
    UseHexOffs = 1;
}



static void OptInfo (const char* Opt attribute ((unused)), const char* Arg)
/* Handle the --info option */
{
    InfoSetName (Arg);
}



static void OptLabelBreak (const char* Opt, const char* Arg)
/* Handle the --label-break option */
{
    /* Convert the argument to a number */
    unsigned long Val = CvtNumber (Opt, Arg);

    /* Check for a valid range */
    RangeCheck (Opt, Val, MIN_LABELBREAK, MAX_LABELBREAK);

    /* Use the value */
    LBreak = (unsigned char) Val;
}



static void OptMnemonicColumn (const char* Opt, const char* Arg)
/* Handle the --mnemonic-column option */
{
    /* Convert the argument to a number */
    unsigned long Val = CvtNumber (Opt, Arg);

    /* Check for a valid range */
    RangeCheck (Opt, Val, MIN_MCOL, MAX_MCOL);

    /* Use the value */
    MCol = (unsigned char) Val;
}



static void OptMultiPass (const char* Opt attribute ((unused)),
                         const char* Arg attribute ((unused)))
/* Handle the --multi-pass option */
{
    MultiPass = 1;
}



static void OptPageLength (const char* Opt attribute ((unused)), const char* Arg)
/* Handle the --pagelength option */
{
    int Len = atoi (Arg);
    if (Len != 0) {
        RangeCheck (Opt, Len, MIN_PAGE_LEN, MAX_PAGE_LEN);
    }
    PageLength = Len;
}



static void OptStartAddr (const char* Opt, const char* Arg)
/* Set the default start address */
{
    StartAddr = (uint32_t) CvtNumber (Opt, Arg);
    HaveStartAddr = 1;
}



static void OptSyncLines (const char* Opt attribute ((unused)),
                          const char* Arg attribute ((unused)))
/* Handle the --sync-lines option */
{
    SyncLines = 1;
}



static void OptTextColumn (const char* Opt, const char* Arg)
/* Handle the --text-column option */
{
    /* Convert the argument to a number */
    unsigned long Val = CvtNumber (Opt, Arg);

    /* Check for a valid range */
    RangeCheck (Opt, Val, MIN_TCOL, MAX_TCOL);

    /* Use the value */
    TCol = (unsigned char) Val;
}



static void OptVerbose (const char* Opt attribute ((unused)),
                        const char* Arg attribute ((unused)))
/* Increase verbosity */
{
    ++Verbosity;
}



static void OptVersion (const char* Opt attribute ((unused)),
                        const char* Arg attribute ((unused)))
/* Print the disassembler version */
{
    fprintf (stderr, "%s V%s\n", ProgName, GetVersionAsString ());
    exit (EXIT_SUCCESS);
}



static unsigned HandleChangedLength(const OpcDesc* D, unsigned PC)
/* Instructions that have flSizeChanges set may use a different size than what
** the table says. This function adjusts the PC accordingly, so after this only
** the size from the table needs to be added to make up for the correct value
*/
{
    if (D->Flags & flSizeChanges) {
        if (CPU == CPU_45GS02) {
            if (D->Handler == OH_Implicit_42_45GS02) {
                if (GetCodeByte (PC+1) == 0x42) {
                    /* NEG:NEG prefix (0x42 0x42) */
                    unsigned opc = GetCodeByte (PC+2);
                    if (opc == 0xea) {
                        /* 42 42 ea */
                        if ((GetCodeByte (PC+3) & 0x1f) == 0x12) {
                            PC += 4;
                        }
                    } else {
                        /* 42 42 xx */
                        const OpcDesc* ED = &OpcTable_45GS02_extended[opc];
                        if (ED->Handler != OH_Illegal) {
                            PC += (ED->Size - 1);
                        }
                    }
                }
            } else if (D->Handler == OH_Implicit_ea_45GS02) {
                /* NOP prefix (0xea) */
                if ((GetCodeByte (PC+1) & 0x1f) == 0x12) {
                    PC += 2;
                }
            }
        } else if (CPU == CPU_65816) {
            if ((D->Handler == OH_Immediate65816M &&
                GetAttr (PC) & atMem16) ||
                (D->Handler == OH_Immediate65816X &&
                GetAttr (PC) & atIdx16)) {
                PC++;
            }
        }
    }
    return PC;
}



static void OneOpcode (unsigned RemainingBytes)
/* Disassemble one opcode */
{
    uint32_t I;
    uint32_t OldPC = PC;

    /* Get the opcode from the current address */
    uint8_t OPC = GetCodeByte (PC);

    /* Get the opcode description for the opcode byte */
    const OpcDesc* D = &OpcTable[OPC];

    /* Get the output style for the current PC */
    attr_t Style = GetStyleAttr (PC);

    /* If a segment begins here, then name that segment.
    ** Note that the segment is named even if its code is being skipped,
    ** because some of its later code might not be skipped.
    */
    if (IsSegmentStart (PC)) {
        StartSegment (GetSegmentStartName (PC), GetSegmentAddrSize (PC));
    }

    /* If we have a label at this address, output the label and an attached
    ** comment, provided that we aren't in a skip area.
    */
    if (Style != atSkip && MustDefLabel (PC)) {
        const char* Comment = GetComment (PC);
        if (Comment) {
            UserComment (Comment);
        }
        DefLabel (GetLabelName (PC));
    }

    /* Check...
    **   - ...if we have enough bytes remaining for the code at this address.
    **   - ...if the current instruction is valid for the given CPU.
    **   - ...if there is no label somewhere between the instruction bytes.
    **   - ...if there is no segment change between the instruction bytes.
    ** If any one of those conditions is false, switch to data mode.
    */
    if (Style == atDefault) {
        if (D->Size > RemainingBytes) {
            Style = atIllegal;
            MarkAddr (PC, Style);
        } else if (D->Flags & flIllegal) {
            Style = atIllegal;
            MarkAddr (PC, Style);
        } else {
            for (I = PC + D->Size; --I > PC; ) {
                if (HaveLabel (I) || IsSegmentStart (I)) {
                    Style = atIllegal;
                    MarkAddr (PC, Style);
                    break;
                }
            }
            for (I = 0; I < D->Size - 1u; ++I) {
                if (IsSegmentEnd (PC + I)) {
                    Style = atIllegal;
                    MarkAddr (PC, Style);
                    break;
                }
            }
        }
    }

    /* Disassemble the line */
    switch (Style) {

        case atDefault:
            if (CPU == CPU_65816) {
                DataByteLine (1);
                ++PC;
            } else {
                D->Handler (D);
                PC = HandleChangedLength (D, PC);
                PC += D->Size;
            }
            break;

        case atCode:
            /* Beware: If we don't have enough bytes left to disassemble the
            ** following insn, fall through to byte mode.
            */
            if (D->Size <= RemainingBytes) {
                if (CPU == CPU_65816) {
                    const unsigned AddrMode = GetAttr (PC) & at65816Mask;
                    if (PrevAddrMode != AddrMode) {
                        if ((PrevAddrMode & atMem8) != (AddrMode & atMem8) ||
                            (PrevAddrMode & atMem16) != (AddrMode & atMem16)) {
                            OutputMFlag (!!(AddrMode & atMem8));
                        }
                        if ((PrevAddrMode & atIdx8) != (AddrMode & atIdx8) ||
                            (PrevAddrMode & atIdx16) != (AddrMode & atIdx16)) {
                            OutputXFlag (!!(AddrMode & atIdx8));
                        }

                        PrevAddrMode = AddrMode;
                    }
                }

                /* Output labels within the next insn */
                for (I = 1; I < D->Size; ++I) {
                    ForwardLabel (I);
                }
                /* Output the insn */
                D->Handler (D);

                PC = HandleChangedLength (D, PC);
                PC += D->Size;
                break;
            }
            /* FALLTHROUGH */

        case atByteTab:
            ByteTable ();
            break;

        case atDByteTab:
            DByteTable ();
            break;

        case atWordTab:
            WordTable ();
            break;

        case atDWordTab:
            DWordTable ();
            break;

        case atAddrTab:
            AddrTable ();
            break;

        case atRtsTab:
            RtsTable ();
            break;

        case atTextTab:
            TextTable ();
            break;

        case atSkip:
            ++PC;
            break;

        default:
            DataByteLine (1);
            ++PC;
            break;
    }

    /* Change back to the default CODE segment if
    ** a named segment stops at the current address.
    */
    for (I = PC - OldPC; I > 0; --I) {
        if (IsSegmentEnd (PC - I)) {
            EndSegment ();
            break;
        }
    }
}



static void OnePass (void)
/* Make one pass through the code */
{
    uint32_t Count;

    PrevAddrMode = 0;

    /* Disassemble until nothing left */
    while ((Count = GetRemainingBytes ()) > 0) {
        OneOpcode (Count);
    }
}



static void Disassemble (void)
/* Disassemble the code */
{
    /* Preparation pass */
    Pass = PASS_PREP;
    OnePass ();

    /* If the --multi-pass option is given, repeat this pass until we have no
    ** new labels.
    */
    if (MultiPass) {
        unsigned long LabelCount = GetLabelCount ();
        unsigned Passes = 1;
        while (1) {
            unsigned long NewLabelCount;
            ResetCode ();
            OnePass ();
            CHECK(++Passes <= 4096);            /* Safety measure */
            NewLabelCount = GetLabelCount ();
            if (NewLabelCount <= LabelCount) {
                break;
            }
            LabelCount = NewLabelCount;
        }
        if (Debug) {
            printf ("Run %u preparation passes to resolve labels\n", Passes);
        }
    }

    Output ("---------------------------");
    LineFeed ();

    /* Final pass */
    Pass = PASS_FINAL;
    ResetCode ();
    OutputSettings ();
    DefOutOfRangeLabels ();
    OnePass ();
}



int main (int argc, char* argv [])
/* Assembler main program */
{
    /* Program long options */
    static const LongOpt OptTab[] = {
        { "--argument-column",  1,      OptArgumentColumn       },
        { "--bytes-per-line",   1,      OptBytesPerLine         },
        { "--comment-column",   1,      OptCommentColumn        },
        { "--comments",         1,      OptComments             },
        { "--cpu",              1,      OptCPU                  },
        { "--debug",            0,      OptDebug                },
        { "--debug-info",       0,      OptDebugInfo            },
        { "--formfeeds",        0,      OptFormFeeds            },
        { "--help",             0,      OptHelp                 },
        { "--hexoffs",          0,      OptHexOffs              },
        { "--info",             1,      OptInfo                 },
        { "--label-break",      1,      OptLabelBreak           },
        { "--mnemonic-column",  1,      OptMnemonicColumn       },
        { "--multi-pass",       0,      OptMultiPass            },
        { "--pagelength",       1,      OptPageLength           },
        { "--start-addr",       1,      OptStartAddr            },
        { "--sync-lines",       0,      OptSyncLines            },
        { "--text-column",      1,      OptTextColumn           },
        { "--verbose",          0,      OptVerbose              },
        { "--version",          0,      OptVersion              },
    };

    unsigned I;
    time_t T;

    /* Initialize the cmdline module */
    InitCmdLine (&argc, &argv, "da65");

    /* Check the parameters */
    I = 1;
    while (I < ArgCount) {

        /* Get the argument */
        const char* Arg = ArgVec[I];

        /* Check for an option */
        if (Arg [0] == '-') {
            switch (Arg [1]) {

                case '-':
                    LongOption (&I, OptTab, sizeof(OptTab)/sizeof(OptTab[0]));
                    break;

                case 'd':
                    if (Arg[2] == '\0') {
                        OptDebug (Arg, 0);
                    } else {
                        UnknownOption (Arg);
                    }
                    break;

                case 'g':
                    OptDebugInfo (Arg, 0);
                    break;

                case 'h':
                    OptHelp (Arg, 0);
                    break;

                case 'i':
                    OptInfo (Arg, GetArg (&I, 2));
                    break;

                case 'm':
                    OptMultiPass (Arg, 0);
                    break;

                case 'o':
                    OutFile = GetArg (&I, 2);
                    break;

                case 'v':
                    OptVerbose (Arg, 0);
                    break;

                case 'S':
                    OptStartAddr (Arg, GetArg (&I, 2));
                    break;

                case 's':
                    OptSyncLines (Arg, 0);
                    break;

                case 'V':
                    OptVersion (Arg, 0);
                    break;

                default:
                    UnknownOption (Arg);
                    break;

            }
        } else {
            /* Filename. Check if we already had one */
            if (InFile) {
                fprintf (stderr, "%s: Don't know what to do with '%s'\n",
                         ProgName, Arg);
                exit (EXIT_FAILURE);
            } else {
                InFile = Arg;
            }
        }

        /* Next argument */
        ++I;
    }

    /* Try to read the info file */
    ReadInfoFile ();

    /* Must have an input file */
    if (InFile == 0) {
        AbEnd ("No input file");
    }

    /* Check the formatting options for reasonable values. Note: We will not
    ** really check that they make sense, just that they aren't complete
    ** garbage.
    */
    if (MCol >= ACol) {
        AbEnd ("mnemonic-column value must be smaller than argument-column value");
    }
    if (ACol >= CCol) {
        AbEnd ("argument-column value must be smaller than comment-column value");
    }
    if (CCol >= TCol) {
        AbEnd ("comment-column value must be smaller than text-column value");
    }

    /* If no CPU given, use the default CPU */
    if (CPU == CPU_UNKNOWN) {
        CPU = CPU_6502;
    }

    /* Get the current time and convert it to string so it can be used in
    ** the output page headers.
    */
    T = time (0);
    strftime (Now, sizeof (Now), "%Y-%m-%d %H:%M:%S", localtime (&T));

    /* Load the input file */
    LoadCode ();

    /* Open the output file */
    OpenOutput (OutFile);

    /* Disassemble the code */
    Disassemble ();

    /* Close the output file */
    CloseOutput ();

    /* Done */
    return EXIT_SUCCESS;
}
