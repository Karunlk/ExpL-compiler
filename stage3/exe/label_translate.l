%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

    #include "label_translate.c"

    FILE *input_file;
    FILE *output_file;

    int address;
%}

%%

%%

void firstPass()
{
    char line[500];
    char label[100];
    int i;

    address = 2056;

    for (i = 0; i < 8; i++)
    {
        if (fgets(line, sizeof(line), input_file) == NULL)
        {
            printf("Invalid input file\n");
            exit(1);
        }
    }

    while (fgets(line, sizeof(line), input_file) != NULL)
    {
        if (sscanf(line, " %99[^:]:", label) == 1 &&
            strchr(line, ':') != NULL)
        {
            addLabel(label, address);
        }
        else
        {
            address += 2;
        }
    }
}


void secondPass()
{
    char line[500];
    char instruction[50];
    char arg1[100];
    char arg2[100];
    char label[100];
    int i;

    rewind(input_file);

    for (i = 0; i < 8; i++)
    {
        fgets(line, sizeof(line), input_file);
        fprintf(output_file, "%s", line);
    }

    while (fgets(line, sizeof(line), input_file) != NULL)
    {
        if (sscanf(line, " %99[^:]:", label) == 1 &&
            strchr(line, ':') != NULL)
        {
            continue;
        }

        if (sscanf(line, " %49s %99[^,], %99s",
                   instruction, arg1, arg2) == 3)
        {
            if (strcmp(instruction, "JZ") == 0 ||
                strcmp(instruction, "JNZ") == 0)
            {
                if (arg2[0] == 'L')
                {
                    fprintf(output_file,
                            "%s %s, %d\n",
                            instruction,
                            arg1,
                            getAddress(arg2));
                }
                else
                {
                    fprintf(output_file, "%s", line);
                }

                continue;
            }
        }

        if (sscanf(line, " %49s %99s",
                   instruction, arg1) == 2)
        {
            if (strcmp(instruction, "JMP") == 0)
            {
                if (arg1[0] == 'L')
                {
                    fprintf(output_file,
                            "%s %d\n",
                            instruction,
                            getAddress(arg1));
                }
                else
                {
                    fprintf(output_file, "%s", line);
                }

                continue;
            }

            if (strcmp(instruction, "CALL") == 0)
            {
                if (arg1[0] >= 'A' && arg1[0] <= 'Z')
                {
                    fprintf(output_file,
                            "%s %d\n",
                            instruction,
                            getAddress(arg1));
                }
                else
                {
                    fprintf(output_file, "%s", line);
                }

                continue;
            }
        }

        fprintf(output_file, "%s", line);
    }
}


int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <input-file>\n", argv[0]);
        return 1;
    }

    input_file = fopen(argv[1], "r");

    if (input_file == NULL)
    {
        perror("Cannot open input file");
        return 1;
    }

    output_file = fopen("output.xsm", "w");

    if (output_file == NULL)
    {
        perror("Cannot open output file");
        fclose(input_file);
        return 1;
    }

    firstPass();

    secondPass();

    fclose(input_file);
    fclose(output_file);

    freeLabels();

    printf("Label translation completed successfully\n");
    printf("Output generated in output.xsm\n");

    return 0;
}


int yywrap()
{
    return 1;
}