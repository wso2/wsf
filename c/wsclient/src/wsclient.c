/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "constants.h"
#include "error.h"
#include "stub.h"
#include "option.h"
#include <axiom.h>

#define BUFF_SIZE 1024
static void 
print_help ();

static void 
print_usage ();

static void 
print_version ();

static void
parse_input(const axutil_env_t *env,
			axis2_char_t **input);


int main (int argc, char **argv)
{
	axutil_env_t *env = NULL;
	char *input[2];
 	char *dest_uri = NULL;
	axutil_array_list_t *array_list;
	axis2_status_t status;
    int ii = 0;
    axis2_char_t *name = NULL;
    axis2_char_t *log_path = "wsclient.log";
    axutil_log_levels_t log_level = AXIS2_LOG_LEVEL_INFO;

	if (argc > 1)
	{
	    if (!strcmp (argv[1], "-h") || !strcmp (argv[1], "--help"))
			print_help ();
		else if (!strcmp (argv[1], "-v") || !strcmp (argv[1], "--version"))
			print_version ();

	    for (ii = 0; ii < argc; ii++)
	    {
            name = NULL;
            if (argv[ii][0] == '-' && argv[ii][1] == '-')
            {
                name = argv[ii]+2;
            }
            if (!axutil_strcmp(name, "log-level"))
            {
                axis2_char_t *level = (char *)argv[++ii];
                if(!axutil_strcmp(level, "user"))
                {
                    log_level = AXIS2_LOG_LEVEL_USER;
                }
                if(!axutil_strcmp(level, "debug"))
                {
                    log_level = AXIS2_LOG_LEVEL_DEBUG;
                }
                if(!axutil_strcmp(level, "info"))
                {
                    log_level = AXIS2_LOG_LEVEL_INFO;
                }
                if(!axutil_strcmp(level, "warning"))
                {
                    log_level = AXIS2_LOG_LEVEL_WARNING;
                }
                if(!axutil_strcmp(level, "error"))
                {
                    log_level = AXIS2_LOG_LEVEL_ERROR;
                }
                if(!axutil_strcmp(level, "critical"))
                {
                    log_level = AXIS2_LOG_LEVEL_CRITICAL;
                }
            }
            else if(!axutil_strcmp(name, "log-path"))
            {
                log_path = (char *)argv[++ii];
            }
        }
	    env = axutil_env_create_all (log_path, log_level);

		AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
						"[wsclient]waiting for input  ");
	    parse_input (env, input);

	    dest_uri = wsclient_destination (argc, argv);
		AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
						"[wsclient]sending request to  %s", dest_uri);

	    array_list = wsclient_process_options (argc, argv, env);
	    status = wsclient_stub_invoke (env, array_list, dest_uri, *input);
		AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[wsclient]wsclient invoked ");
		
		switch (status)
		{
			case WSCLIENT_FAILURE:
			{
				AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
								"[wsclient] wsclient invoke failed, generic!");
				printf ("\nwsclient invoke failed, generic! \n");
			}
			break;
			case WSCLIENT_ERROR_NO_DESTURI:
			{
				AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
								"[wsclient] wsclient invoke failed, no dest_uri !");
				printf ("\nwsclient invoke failed, no dest_uri! \n");
			}
			break;
			case WSCLIENT_SUCCESS:
			{
				AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
								"[wsclient] wsclient invoke Success !");
			}
			break;
		};

	}
	else
	    print_usage ();
	axutil_env_free ((void *)env);
	return 0;
}


static void print_usage ()
{
	printf ("Usage: wsclient [options] [destURI]\n");
	printf ("      See wsclient -h for help.\n");
}

static void print_version ()
{
	static const char *version [] = 
	    {
			("\
\n"),
			("\
             wsclient 1.0     \n"),
			("\
\n"),
			("\
  Licensed to the Apache Software Foundation (ASF) under one or more\n"),
			("\
 contributor license agreements.  See the NOTICE file distributed with\n"),
			("\
 this work for additional information regarding copyright ownership.\n"),
			("\
 The ASF licenses this file to You under the Apache License, Version 2.0\n"),
			("\
 (the \"License\"); you may not use this file except in compliance with\n"),
			("\
 the License.  You may obtain a copy of the License at\n"),
			("\
\n"),
			("\
      http://www.apache.org/licenses/LICENSE-2.0\n"),
			("\
\n"),
			("\
 Unless required by applicable law or agreed to in writing, software\n"),
			("\
 distributed under the License is distributed on an \"AS IS\" BASIS,\n"),
			("\
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"),
			("\
 See the License for the specific language governing permissions and\n"),
			("\
 limitations under the License.\n"),
			("\
\n"),
		};
	int i = 0;
	for (i = 0; i < array_sizeof(version); i++)
	    fputs (version[i], stdout);
	exit (0);
}

static void
parse_input(const axutil_env_t *env,
			axis2_char_t **input)
{
    int c, nc, index;
    axis2_char_t buff[BUFF_SIZE];
    nc = 0;
    index = 0;
    *input = NULL;
    c = getchar();
    while(!feof(stdin) && !ferror(stdin))
    {
        nc++;
        buff[index] = (axis2_char_t) c;
        if(index == (BUFF_SIZE - 2))
        {
            axis2_char_t *tmp = NULL;
            buff[BUFF_SIZE - 1] = '\0';
            tmp = axutil_stracat(env, *input, buff);
            if(*input)
            {
                AXIS2_FREE(env->allocator, *input);
            }
            *input = tmp;
            index = 0;
            c = getchar();
            continue;
        }
        c = getchar();
        index++;
    }
    if(feof(stdin) && !ferror(stdin))
    {
        axis2_char_t *tmp = NULL;
        buff[index] = '\0';
        tmp = axutil_stracat(env, *input, buff);
        if(*input)
        {
            AXIS2_FREE(env->allocator, *input);
        }
        *input = tmp;
    }
}


static void 
print_help ()
{
	static const char *help [] = 
	    {
			("\
wsclient 1.0, command line tool to consume webservices.\n"),
			("\
Usage: wsclient [options] [destURI]\n\n"),
			("\
Mandatory arguments to long options are mandatory for short options too.\n\n"),
			("\
Startup:\n"),
			("\
  -v,  --version                    display the version of WSclientt and exit.\n"),
			("\
  -h,  --help                       print this help.\n"),
			"\n",
			("\
HTTP:\n"),
			("\
       --get                        use get as the HTTP method.\n"),
			("\
       --content-type TYPE          specify the payloads content type.\n"),
			"\n",
			("\
SOAP:\n"),
			("\
       --soap                       use the SOAP binding.\n"),
			("\
       --soap1.1                    use version 1.1 of SOAP.\n"),
			("\
  -a,  --action URI                 use URI as the SOAP action.\n"),
			("\
       --soap-out                   output entire SOAP reply.\n"),
			("\
       --soap-header LINE           add LINE as a SOAP header.\n"),
			("\
       --send-only                  send SOAP request only.\n"),
			("\
       --soap-dump                  output SOAP request.\n"),
			"\n",
			("\
Addressing:\n"),
			("\
       --no-wsa                     don't add WS-Addressing headers.\n"),
			("\
       --to URI                     uses URI as the To address.\n"),
			("\
       --from URI                   uses URI as the From address.\n"),
			("\
       --reply-to URI               uses URI as the ReplyTo address.\n"),
			("\
       --fault-to URI               uses URI as the FaultTo address.\n"),
			("\
       --in-reply-to URI            uses URI as the RelatesTo header value.\n"),
			"\n",
			("\
MTOM:\n"),
			("\
       --no-mtom                    disable mtom.\n"),
			("\
       --xop-in DIR                 DIR is the base directory of the package need to transmit.\n"),
			("\
       --xop-out DIR                DIR is a directory for save attachments receive.\n"),
			"\n",
			("\
Rampart:\n"),
			("\
       --user USERNAME              includes username token with user as the USERNAME.\n"),
			("\
       --password PASSWD            uses PASSWD as the password for username token.\n"),
			("\
       --password-file FILE         uses password from FILE.\n"),
			("\
       --digest                     uses a password digest for the usename token.\n"),
			("\
       --sign-body                  Signs the soap body.\n"),
			("\
       --certificate FILE           X509 Certificate for the sender which can be used for signing.\n"),
			("\
       --key FILE                   private key file FILE, which must be the private key in PEM format.\n"),
			("\
       --recipient-certificate FILE public key associated with FILE, which should be an X509 certificate in PEM format.\n"),
			("\
       --encrypt-payload            Encrypts the contents of the SOAP body.\n"),
			("\
       --algorithmsuite SUITE       algorithmsuite for signature and encryption\n"),
			("\
       --encrypt-signature          Encrypts the signature (when signing before encryption).\n"),
			("\
       --encrypt-before-signing     Encrypts before signing(by default wsclient will sign before encrypting.\n"),
			("\
       --timestamp                  Generates a timestamp element in the security header.\n"),
			("\
       --policy-file FILE           Path to the security policy file.\n"),
			"\n",
			("\
General HTTP Options:\n"),
			("\
       --http-header LINE           add LINE as a HTTP header.\n"),
			("\
       --output-http-headers FILE   output the HTTP headers in the response to FILE.\n"),
			"\n",
			("\
SSL options:\n"),
			("\
       --server-cert PATH           path to certificate file to use with https transport.\n"),
			"\n",
			("\
       --log-path PATH              Path to log file. Default is $WSFC_HOME/logs/wsclient.log\n"),
			"\n",
			("\
       --log-level LEVEL            LEVEL is one of critical, error, warning, info, debug, user. Default is info\n"),
			"\n",
	    };
	int i = 0;
	for (i = 0; i < array_sizeof(help); i++)
	    fputs (help[i], stdout);
	exit (0);
}
