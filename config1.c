/*config.c*/
/*
To compile : gcc -o config config.c -lconfig
To run     : ./config
*/
#include <stdio.h> 
#include <libconfig.h>

void main()
{
config();
}

int config()
{
    config_t cfg;               /*Returns all parameters in this structure */
    config_setting_t *setting;
    const char *str1, *str2;
    int tmp;

    char *config_file_name = "config.txt";

    /*Initialization */
    config_init(&cfg);

    /* Read the file. If there is an error, report it and exit. */
    if (!config_read_file(&cfg, config_file_name))
    {
        printf("\n%s:%d - %s", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return -1;
    }

    /* Get the configuration file name. */
    if (config_lookup_string(&cfg, "filename", &str1))
        printf("\nFile Type: %s", str1);
    else
        printf("\nNo 'filename' setting in configuration file.");

    /*Read the parameter group*/
    setting = config_lookup(&cfg, "params");
    if (setting != NULL)
    {
        /*Read the string*/
        if (config_setting_lookup_string(setting, "param1", &str2))
        {
            printf("\nParam1: %s", str2);
        }
        else
            printf("\nNo 'param1' setting in configuration file.");

        /*Read the integer*/
        if (config_setting_lookup_int(setting, "param2", &tmp))
        {
            printf("\nParam2: %d", tmp);
        }
        else
            printf("\nNo 'param2' setting in configuration file.");

        printf("\n");
    }

    config_destroy(&cfg);
}


