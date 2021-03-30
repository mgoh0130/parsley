/* NAME: Michelle Goh
   NetId: mg2657 */
#include "/c/cs323/Hwk2/parsley.h"
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>

// Write message to stderr using format FORMAT
#define WARN(format,...) fprintf (stderr, "parsley: " format "\n", __VA_ARGS__)

// Write message to stderr using format FORMAT and exit.
#define DIE(format,...)  WARN(format,__VA_ARGS__), exit (EXIT_FAILURE)

int listIndex = 0; //index of token list; keeps place of list during parsing
int listLen = 0; //length of list; determines which parts of token list to parse
int error = 0; //indicate error


// Struct for each token in sequence 
typedef struct token 
{         
	char *text;                  
	int type;                    
}token;

CMD *makeCMD(token **list);
CMD *makeSequence(token **list);
bool isLocalFree(char* string);

CMD *parse (char *line)
{
	listIndex = 0;
	error = 0;
	int length = strlen(line);

	int leftPar = 0;
	int rightPar = 0;
	
	token **tokenList = malloc(sizeof(token*) * (length+1));

	int index = 0;

	for(int i = 0; i < length; i++) 
	{
		int special = true;
		if(isspace(line[i])) // ignore whitespace and continue finding token
		{                          
			continue;
		} 
		else if(line[i] == '\\') //escape next char and continue finding token
		{ 
			if(i+1 < length) //add char to string
			{
				i++;
				special = false;
			}
			else //add backslash
			{     
				token *item = malloc(sizeof(token));
				item->text = "\\";
				item->type = TEXT;

				tokenList[index] = item;
				break;
			}              
			
		}
		else if(line[i] == '#') //everything after # is ignored (comment)
		{                     
			break;
		}

		//text found

		token *item = malloc(sizeof(token));

		bool metaChar = false;

		if(special)
		{
			int meta;
			for(meta = 0; meta < 7; meta++)
			{
				if(line[i] == METACHAR[meta])
				{
					metaChar = true;
					break;
				}
			}
			if(metaChar) //exit for loop or continues don't work
			{	
				if(i+1 < length)
				{
					if(((line[i] != '(') && (line[i] != ')')) &&
					(line[i+1] == METACHAR[meta] || line[i+1] == '>')) //if have next meta char, it is same char or &>
					{

						char *subStr = malloc(sizeof(char)*3);
						memcpy(subStr, &line[i], 2);
						subStr[2] = '\0';

						item->text = subStr;

						if(line[i+1] == '>') //set type according to metachar
						{
							if(line[i] == '>')
							{
								item->type = RED_OUT_APP;
							}
							else
							{
								item->type = RED_OUT_ERR;
							}
						}
						else if(line[i+1] == '<')
						{
							item->type = RED_IN_HERE;
						}
						else if(line[i+1] == '&')
						{
							item->type = SEP_AND;
						}
						else if(line[i+1] == '|')
						{
							item->type = SEP_OR;
						}

						i = i+1; //increment
						tokenList[index] = item;
						index++;
						continue;
					}
					else //redirects, bc next slot should not be empty
					{
						char *subStr = malloc(sizeof(char)*2);
						memcpy(subStr, &line[i], 1);
						subStr[1] = '\0';

						item->text = subStr;

						if(line[i] == '<')
						{
							item->type = RED_IN;
						}
						else if(line[i] == '>')
						{
							item->type = RED_OUT;
						}
						else if(line[i] == '|')
						{
							item->type = PIPE;
						}
						else if(line[i] == '(')
						{
							leftPar++;
							item->type = PAR_LEFT;
						}
						else if(line[i] == ')')
						{
							rightPar++;
							item->type = PAR_RIGHT;
						}
						else if(line[i] == ';')
						{
							item->type = SEP_END;
						}
						else if(line[i] == '&')
						{
							item->type =  SEP_BG;
						}

						tokenList[index] = item;
						index++;
						continue;
					}
				}
				else //next char doesn't exist, isn't redirection
				{
					char *subStr = malloc(sizeof(char)*2);
					memcpy(subStr, &line[i], 1);
					subStr[1] = '\0';

					item->text = subStr;

					if(line[i] == ')')
					{
						rightPar++;
						item->type = PAR_RIGHT;
					}
					else if(line[i] == ';')
					{
						item->type = SEP_END;
					}
					else if(line[i] == '&')
					{
						item->type =  SEP_BG;
					}
					else //missing filename
					{
						fprintf(stderr, "parsley: missing filename\n");
						return NULL;
					}

					tokenList[index] = item;
					index++;
					continue;
				}
			}
		}

		//if not metachar, then it is TEXT

		char buf[length+1]; //char buffer
		int strInd = 1;
		
		buf[0] = line[i];
		buf[1] = '\0';
		
		i++;

		while(i < length) //find where token starts and stops; build string of TEXT
		{
			if(isspace(line[i])) // end of token
			{                          
				item->text = strdup(buf);
				item->type = TEXT;

				tokenList[index] = item;
				index++;
				break;
			}
			else if(line[i] == '\\') //escape next char and continue finding token
			{                     
				if(i+1 < length) //add char to string
				{
					i++;
					buf[strInd] = line[i];
					buf[strInd+1] = '\0';
					
					i++;
					strInd++;
				}
				else //add backslash
				{
					buf[strInd] = line[i];
					buf[strInd+1] = '\0';

					i++;
					
					strInd++;
					break;
				}
			}
			else //can be metachar
			{
				for(int meta = 0; meta < 7; meta++)
				{
					if(line[i] == METACHAR[meta])
					{
						metaChar = true;
						break;
					}
				}
				
				if(metaChar)
				{
					item->text = strdup(buf);
					item->type = TEXT;

					tokenList[index] = item;
					index++;
					break;
				}
				else //not metachar, so add
				{
					buf[strInd] = line[i];
					buf[strInd+1] = '\0';

					i++;
					strInd++;
				}
			}
		}

		if(i >= length) //finished
		{
			item->text = strdup(buf);
			item->type = TEXT;

			tokenList[index] = item;
			index++;
			break;
		}

		i--; //decrement b/c the for loop increments for us
	}

	listLen = index; //last index of list plus one is size of list
	if(listLen == 0)
	{
		free(tokenList);
		return NULL;
	}

	if(leftPar != rightPar)
	{
		for(int f = 0; f < listLen; f++)
		{

			free(tokenList[f]->text);


			free(tokenList[f]);
		}
		free(tokenList);
		fprintf(stderr, "parse: uneven parans\n");
		return NULL;
	}

	CMD *tree = makeCMD(tokenList);

	if(error == ERROR)
	{
		for(int f = 0; f < listLen; f++)
		{

		if(tokenList[f]->type != TEXT)
		{
			free(tokenList[f]->text);
		}
		
		free(tokenList[f]);
		}
		free(tokenList);
		return NULL;
	}


	for(int f = 0; f < listLen; f++)
	{
		if(tokenList[f]->type == RED_IN_HERE)
		{
			free(tokenList[f]->text);
			free(tokenList[f]);
			if(f+1 < listLen)
			{
				free(tokenList[f+1]->text);
				f++;
			}
		}
		else if(tokenList[f]->type != TEXT)
		{
			free(tokenList[f]->text);
		}
		else if(isLocalFree(tokenList[f]->text))
		{
			free(tokenList[f]->text);
		}
		
		free(tokenList[f]);
	}

	free(tokenList);

	return tree;
}

bool isLocalFree(char* string)
{
	bool goodName = false;
	bool var = false;

	for(int i = 0; i < strlen(string); i++)
	{
		goodName = false;

			if(string[i] == '=') //split NAME and VALUE
			{
				var = true;
				break;
			}
			
			for(int j = 0; j < strlen(VARCHR); j++) //check every char
			{
				if(string[i] == VARCHR[j]) //check valid NAME
				{
					goodName = true;
					break;
				}
			}

			if(!goodName)
			{
				return false; //not NAME=VALUE
			}
	}
	return var;
}

	bool isLocal(token* item, char **NAME, char **VALUE)
	{
		char *string = item->text;
		if(isdigit(string[0]))
		{
			return false;
		}
		else
		{
			int partition = -1;
			bool goodName;

			for(int i = 0; i < strlen(string); i++)
			{
				goodName = false;

			if(string[i] == '=') //split NAME and VALUE
			{
				partition = i;
				break;
			}
			
			for(int j = 0; j < strlen(VARCHR); j++) //check every char
			{
				if(string[i] == VARCHR[j]) //check valid NAME
				{
					goodName = true;
					break;
				}
			}

			if(!goodName)
			{
				return false; //not NAME=VALUE
			}
		}

		if(partition > 0)
		{
			char *name = malloc(sizeof(char)*(partition+1)); //partition chars in NAME
			memcpy(name, string, partition);
			name[partition] = '\0';

			*NAME = name; //set name of variable

			int stringLen = strlen(string);

			char *value = malloc(sizeof(char)*(stringLen - partition));

			if(partition != stringLen-1)
			{
			memcpy(value, &string[partition+1], stringLen - partition - 1);
			value[partition] = '\0';

			*VALUE = value;
		}
		else
		{
			value [0] = '\0';
			*VALUE = value;
		}

			return true;
		}
		else
		{
			return false;
		}
	}
}

bool isRedirect(token **list)
{
	if(RED_OP(list[listIndex]->type)) //redirection symbol
	{
		if((listIndex + 1 < listLen) && (list[listIndex+1]->type == TEXT)) //valid filename
		{
			return true;
		}
		else
		{
			error = ERROR; //invalid filename
			fprintf(stderr, "parsley: improper filename\n");
			return false;
		}
	}
	else
	{
		return false;
	}
}

CMD *makeSimple(token **list)
{
	CMD *tree = mallocCMD(SIMPLE, NULL, NULL);

	//check if current token in list is part of
	//a prefix

	char *NAME = NULL;
	char *VALUE = NULL;

	char **variables = malloc(sizeof(char*)*(listLen+1));
	char **varValues = malloc(sizeof(char*)*(listLen+1));
	int locals = 0;

	while(listIndex < listLen && (isLocal(list[listIndex], &NAME, &VALUE) || isRedirect(list))) //subsequent tokens
	{
		if(!RED_OP(list[listIndex]->type)) //local
		{
			locals++;

			variables[locals-1] = NAME;
			varValues[locals-1] = VALUE;

			listIndex++;
		}
		else
		{
			if(error == 0) //no error
			{
				if(list[listIndex]->type == RED_IN) //set attributes of tree
				{
					if(tree->fromType != NONE)
					{
						error = ERROR;
						fprintf(stderr, "parsley: multiple input redirects\n");

						for(int f = 0; f < locals; f++)
						{
							free(variables[f]);
							free(varValues[f]);
						}

						free(variables);
						free(varValues);
						return freeCMD(tree);
					}
					else
					{
						tree->fromType = RED_IN;
						tree->fromFile = list[listIndex+1]->text;
					}
				}
				else if(list[listIndex]->type == RED_OUT) 
				{
					if(tree->toType != NONE)
					{
						error = ERROR;
						fprintf(stderr, "parsley: multiple output redirects\n");
						
						for(int f = 0; f < locals; f++)
						{
							free(variables[f]);
							free(varValues[f]);
						}

						free(variables);
						free(varValues);
						
						return freeCMD(tree);
					}
					else
					{
						tree->toType = RED_OUT;
						tree->toFile = list[listIndex+1]->text;
					}
				}
				else if(list[listIndex]->type == RED_OUT_APP) 
				{
					if(tree->toType != NONE)
					{
						error = ERROR;
						fprintf(stderr, "parsley: multiple output redirects\n");

						
						for(int f = 0; f < locals; f++)
						{
							free(variables[f]);
							free(varValues[f]);
						}

						free(variables);
						free(varValues);
						
						return freeCMD(tree);
					}
					else
					{
						tree->toType = RED_OUT_APP;
						tree->toFile = list[listIndex+1]->text;
					}
				}
				else if(list[listIndex]->type == RED_IN_HERE) //set attributes of tree; HERE
				{
					if(tree->fromType != NONE)
					{
						error = ERROR;
						fprintf(stderr, "parsley: multiple input redirects\n");

						
						for(int f = 0; f < locals; f++)
						{
							free(variables[f]);
							free(varValues[f]);
						}

						free(variables);
						free(varValues);
						
						return freeCMD(tree);
					}
					else
					{
						tree->fromType = RED_IN_HERE;

						char *line = NULL;                         
						size_t nLine = 0;  

						int hereInputLen = strlen(list[listIndex+1]->text);

						char *hereInput = malloc(sizeof(char*)*(hereInputLen+2));

						strcpy(hereInput, list[listIndex+1]->text);
						hereInput[hereInputLen] = '\n';
						hereInput[hereInputLen+1] = '\0';

						char *s = malloc(sizeof(char));
						char* temp;
						s[0] = '\0';

						while(getline (&line,&nLine, stdin) > 0) // getline failed
						{
							if(strcmp(line, hereInput) == 0)
							{
								break;
							}	
							
							temp = malloc((strlen(line)+strlen(s)+2)*sizeof(char)); 
							strcpy(temp, s);

							strcat(temp, line);
							free(s);

							s = temp;
						}

						tree->fromFile = s;
						free(line);
						free(hereInput);
					}
				}
				else
				{
					error = ERROR;
					printf("redirect symbol not found\n");
					exit(0);
				}
				listIndex = listIndex + 2; //consume the redirection and filename
			}
		}
	}

	//next token should be TEXT, if not, then this is not a simple

	if(listIndex >= listLen)
	{
		error = ERROR;
		fprintf(stderr, "parsley: NULL command\n");
		
		for(int f = 0; f < locals; f++)
		{
			free(variables[f]);
			free(varValues[f]);
		}

		free(variables);
		free(varValues);
		return freeCMD(tree);
	}		

	if(listIndex < listLen && list[listIndex]->type == TEXT)
	{
		int numArgs = 1;
		char **args = malloc(sizeof(char*)*(listLen+1)); //max number of args
		args[numArgs-1] = list[listIndex]->text; //consume token
		
		listIndex++;

		while(listIndex < listLen && ((list[listIndex]->type == TEXT) || isRedirect(list)))  //subsequent tokens suffix
		{
			if(!RED_OP(list[listIndex]->type))
			{
				numArgs++;
				args[numArgs-1] = list[listIndex]->text;
				listIndex++;
			}
			else
			{
				if(error == 0) //no error
				{
					if(list[listIndex]->type == RED_IN) //set attributes of tree
					{
						if(tree->fromType != NONE)
						{
							error = ERROR;
							fprintf(stderr, "parsley: multiple input redirects\n");
							
							for(int f = 0; f < locals; f++)
							{
								free(variables[f]);
								free(varValues[f]);
							}

							free(variables);
							free(varValues);
							
							for(int a = 0; a < numArgs; a++)
							{
								free(args[a]);
							}

							free(args);

							return freeCMD(tree);
						}
						else
						{
							tree->fromType = RED_IN;
							tree->fromFile = list[listIndex+1]->text;
						}
					}
					else if(list[listIndex]->type == RED_OUT) 
					{
						if(tree->toType != NONE)
						{
							error = ERROR;
							fprintf(stderr, "parsley: multiple output redirects\n");
							for(int f = 0; f < locals; f++)
							{
								free(variables[f]);
								free(varValues[f]);
							}

							free(variables);
							free(varValues);
							
							for(int a = 0; a < numArgs; a++)
							{
								free(args[a]);
							}

							free(args);
							return freeCMD(tree);
						}
						else
						{
							tree->toType = RED_OUT;
							tree->toFile = list[listIndex+1]->text;
						}
					}
					else if(list[listIndex]->type == RED_OUT_APP) 
					{
						if(tree->toType != NONE)
						{
							error = ERROR;
							fprintf(stderr, "parsley: multiple output redirects\n");

							for(int f = 0; f < locals; f++)
							{
								free(variables[f]);
								free(varValues[f]);
							}

							free(variables);
							free(varValues);
							
							for(int a = 0; a < numArgs; a++)
							{
								free(args[a]);
							}

							free(args);
							return freeCMD(tree);
						}
						else
						{
							tree->toType = RED_OUT_APP;
							tree->toFile = list[listIndex+1]->text;
						}
					}
					else if(list[listIndex]->type == RED_IN_HERE) //set attributes of tree; HERE
					{

						if(tree->fromType != NONE)
						{
							error = ERROR;
							fprintf(stderr, "parsley: multiple input redirects\n");

							for(int f = 0; f < locals; f++)
							{
								free(variables[f]);
								free(varValues[f]);
							}

							free(variables);
							free(varValues);
							
							for(int a = 0; a < numArgs; a++)
							{
								free(args[a]);
							}

							free(args);
							return freeCMD(tree);
						}
						else
						{
							tree->fromType = RED_IN_HERE;

							char *line = NULL;                         
							size_t nLine = 0;  

							int hereInputLen = strlen(list[listIndex+1]->text);

							char *hereInput = malloc(sizeof(char*)*(hereInputLen+2));

							strcpy(hereInput, list[listIndex+1]->text);
							hereInput[hereInputLen] = '\n';
							hereInput[hereInputLen+1] = '\0';

							char *s = malloc(sizeof(char));
							char* temp;
							s[0] = '\0';

						while(getline (&line,&nLine, stdin) > 0) // getline failed
						{
							if(strcmp(line, hereInput) == 0)
							{
								break;
							}	
							
							temp = malloc((strlen(line)+strlen(s)+2)*sizeof(char)); 
							strcpy(temp, s);

							strcat(temp, line);
							free(s);
							s = temp;
						}

						tree->fromFile = s;
						free(line);
						free(hereInput);
					}
				}
				else
				{
					error = ERROR;
					printf("redirect symbol not found\n");
					exit(0);
				}
					listIndex = listIndex + 2; //consume the redirection and filename
				}
			}
		}

		args[numArgs] = '\0';
		free(tree->argv);
		tree->argv = args;
		tree->argc = numArgs;

		if(locals > 0)
		{
			variables[locals] = '\0';
			varValues[locals] = '\0';

			tree->locVar = variables;
			tree->locVal = varValues;
			tree->nLocal = locals;
		}
		else
		{
			for(int f = 0; f < locals; f++)
			{
				free(variables[f]);
				free(varValues[f]);
			}

			free(variables);
			free(varValues);


		}

		return tree;

	}

	for(int f = 0; f < locals; f++)
	{
		free(variables[f]);
		free(varValues[f]);
	}

	free(variables);
	free(varValues);
	if(error == 0 && tree == NULL)
	{
		return freeCMD(tree);
	}
	else
	{
		return NULL; //not able to make simple
	}
}

CMD *makeStage(token **list)
{
	int save = listIndex;
	CMD *tree = makeSimple(list);
	if(tree != NULL)
	{
		return tree;
	}
	else //make subcmd CMD or error
	{
		if(error != 0)
		{
			return freeCMD(tree);
		}
		else
		{
		//check if current token in list is part of
		//a prefix

			listIndex = save;

			tree = mallocCMD(SUBCMD, NULL, NULL);

			char *NAME = NULL;
			char *VALUE = NULL;

			char **variables = malloc(sizeof(char*)*(listLen+1));
			char **varValues = malloc(sizeof(char*)*(listLen+1));
			int locals = 0;

			while(listIndex < listLen && (isLocal(list[listIndex], &NAME, &VALUE) || isRedirect(list))) //PREFIX
			{
				if(!RED_OP(list[listIndex]->type)) //local
				{
					locals++;

					variables[locals-1] = NAME;
					varValues[locals-1] = VALUE;

					listIndex++;
				}
				else
				{
					if(error == 0) //no error
					{
						if(list[listIndex]->type == RED_IN) //set attributes of tree
						{
							if(tree->fromType != NONE)
							{
								error = ERROR;
								fprintf(stderr, "parsley: multiple input redirects\n");

								free(variables);
								free(varValues);
								return freeCMD(tree);
							}
							else
							{
								tree->fromType = RED_IN;
								tree->fromFile = list[listIndex+1]->text;
							}
						}
						else if(list[listIndex]->type == RED_OUT) 
						{	
							if(tree->toType != NONE)
							{
								error = ERROR;
								fprintf(stderr, "parsley: multiple output redirects\n");

								
								free(variables);
								free(varValues);

								return freeCMD(tree);
							}
							else
							{
								tree->toType = RED_OUT;
								tree->toFile = list[listIndex+1]->text;
							}
						}
						else if(list[listIndex]->type == RED_OUT_APP) 
						{
							if(tree->toType != NONE)
							{
								error = ERROR;
								fprintf(stderr, "parsley: multiple output redirects\n");

								
								free(variables);
								free(varValues);
								
								return freeCMD(tree);
							}
							else
							{
								tree->toType = RED_OUT_APP;
								tree->toFile = list[listIndex+1]->text;
							}
						}
						else if(list[listIndex]->type == RED_IN_HERE) //set attributes of tree; HERE
						{
							if(tree->fromType != NONE)
							{
								error = ERROR;
								fprintf(stderr, "parsley: multiple input redirects\n");

								
								free(variables);
								free(varValues);
								
								return freeCMD(tree);
							}
							else
							{
								tree->fromType = RED_IN_HERE;

								char *line = NULL;                         
								size_t nLine = 0;  

								int hereInputLen = strlen(list[listIndex+1]->text);

								char *hereInput = malloc(sizeof(char*)*(hereInputLen+2));

								strcpy(hereInput, list[listIndex+1]->text);
								hereInput[hereInputLen] = '\n';
								hereInput[hereInputLen+1] = '\0';

								char *s = malloc(sizeof(char));
								char* temp;
								s[0] = '\0';

						while(getline (&line,&nLine, stdin) > 0) // getline failed
						{
							if(strcmp(line, hereInput) == 0)
							{
								break;
							}	
							
							temp = malloc((strlen(line)+strlen(s)+2)*sizeof(char)); 
							strcpy(temp, s);

							strcat(temp, line);
							free(s);

							s = temp;
						}

						tree->fromFile = s;
						free(line);
						free(hereInput);
					}
				}
				else
				{
					error = ERROR;
					printf("redirect symbol not found\n");
					exit(0);
				}
				listIndex = listIndex + 2; //consume the redirection and filename
			}
		}
	}

			//next token should be ()

	if(listIndex >= listLen)
	{
		error = ERROR;
		fprintf(stderr, "parsley: NULL command\n");

		free(variables);
		free(varValues);
		return freeCMD(tree);
	}		

			//printf("1%s\n", list[listIndex]->text);



			if(listIndex < listLen && list[listIndex]->type == PAR_LEFT) //command
			{
				listIndex++;

				CMD *tree2 = makeCMD(list);

				if((error == ERROR) || list[listIndex]->type != PAR_RIGHT)
				{
					free(tree2);
					return freeCMD(tree);
				}
				else
				{
					listIndex++;
					tree->left = tree2;
					tree->right = NULL;
					
				}
			}
			else if(list[listIndex]->type != PAR_RIGHT) //not a command
			{
				error = ERROR;
				fprintf(stderr, "parsley: Unable to make simple or subcmd\n");
				free(variables);
				free(varValues);
				return freeCMD(tree);
			}

			//redList
			while(listIndex < listLen && isRedirect(list)) //subsequent tokens
			{

				if(error == 0) //no error
				{

						if(list[listIndex]->type == RED_IN) //set attributes of tree
						{

							if(tree->fromType != NONE)
							{
								error = ERROR;
								fprintf(stderr, "parsley: multiple input redirects\n");
								
								free(variables);
								free(varValues);
								return freeCMD(tree);
							}
							else
							{
								tree->fromType = RED_IN;
								tree->fromFile = list[listIndex+1]->text;
							}
						}
						else if(list[listIndex]->type == RED_OUT) 
						{	
							if(tree->toType != NONE)
							{
								error = ERROR;
								fprintf(stderr, "parsley: multiple output redirects\n");

								
								free(variables);
								free(varValues);
								
								return freeCMD(tree);
							}
							else
							{
								tree->toType = RED_OUT;
								tree->toFile = list[listIndex+1]->text;
							}
						}
						else if(list[listIndex]->type == RED_OUT_APP) 
						{
							if(tree->toType != NONE)
							{
								error = ERROR;
								fprintf(stderr, "parsley: multiple output redirects\n");


								free(variables);
								free(varValues);

								return freeCMD(tree);
							}
							else
							{
								tree->toType = RED_OUT_APP;
								tree->toFile = list[listIndex+1]->text;
							}
						}
						else if(list[listIndex]->type == RED_IN_HERE) //set attributes of tree; HERE
						{
							if(tree->fromType != NONE)
							{
								error = ERROR;
								fprintf(stderr, "parsley: multiple input redirects\n");

								
								free(variables);
								free(varValues);

								return freeCMD(tree);
							}
							else
							{
								tree->fromType = RED_IN_HERE;

								char *line = NULL;                         
								size_t nLine = 0;  

								int hereInputLen = strlen(list[listIndex+1]->text);

								char *hereInput = malloc(sizeof(char*)*(hereInputLen+2));

								strcpy(hereInput, list[listIndex+1]->text);
								hereInput[hereInputLen] = '\n';
								hereInput[hereInputLen+1] = '\0';

								char *s = malloc(sizeof(char));
								char* temp;
								s[0] = '\0';

						while(getline (&line,&nLine, stdin) > 0) // getline failed
						{
							if(strcmp(line, hereInput) == 0)
							{
								break;
							}	
							
							temp = malloc((strlen(line)+strlen(s)+2)*sizeof(char)); 
							strcpy(temp, s);

							strcat(temp, line);
							free(s);

							s = temp;
						}

						tree->fromFile = s;
						free(line);
						free(hereInput);

					}
				}
				else
				{
					error = ERROR;
					printf("redirect symbol not found\n");
					exit(0);
				}
						listIndex = listIndex + 2; //consume the redirection and filename
					}
				}

				if(listIndex < listLen && 
					((list[listIndex]->type == PAR_LEFT) || list[listIndex]->type == TEXT))
				{
					error = ERROR;
					fprintf(stderr, "parsley: invalid following subcmd\n");

					free(variables); //no locals, don't use malloced memory
					free(varValues);
					return freeCMD(tree);
				}
				else
				{
					if(locals > 0)
					{
						variables[locals] = '\0';
						varValues[locals] = '\0';

						tree->locVar = variables;
						tree->locVal = varValues;
						tree->nLocal = locals;
					}
					else
					{
					free(variables); //no locals, don't use malloced memory
					free(varValues);
				}
				return tree;
			}
		}

	}
}

CMD *makePipeline(token **list)
{
	CMD *tree;
	tree = makeStage(list);

	if(error == ERROR || tree == NULL)
	{
		return freeCMD(tree);
	}

	while(listIndex < listLen && ((tree != NULL) && (list[listIndex]->type == PIPE)))
	{
		listIndex++;

		if(listIndex >= listLen)
		{
			error = ERROR;
			fprintf(stderr, "parsley: NULL command pipe\n");
			return freeCMD(tree);
		}		

		CMD *tree2 = makeStage(list);

		if(error == ERROR || tree2 == NULL)
		{
			freeCMD(tree2);
			return freeCMD(tree);
		}
		else
		{
			CMD *treeBig = mallocCMD(PIPE, NULL, NULL);

			treeBig->left = tree;
			treeBig->right = tree2;

			tree = treeBig;
		}
	}
	return tree;
}

CMD *makeAndOr(token **list)
{
	CMD *tree = makePipeline(list);

	if(error == ERROR || tree == NULL)
	{
		return freeCMD(tree);
	}

	while(((tree != NULL) && listIndex < listLen) && 
		((list[listIndex]->type == SEP_AND) || (list[listIndex]->type == SEP_OR)))
	{
		CMD *treeBig;

		if(list[listIndex]->type == SEP_AND)
		{
			treeBig = mallocCMD(SEP_AND, NULL, NULL);
		}
		else
		{
			treeBig = mallocCMD(SEP_OR, NULL, NULL);
		}

		listIndex++;

		if(listIndex >= listLen)
		{
			error = ERROR;
			fprintf(stderr, "parsley: NULL command andor\n");

			freeCMD(tree);
			return freeCMD(treeBig);
		}	

		CMD *tree2 = makePipeline(list);

		if(error == ERROR || tree2 == NULL)
		{
			freeCMD(tree2);
			return freeCMD(tree);
		}
		else
		{
			treeBig->left = tree;
			treeBig->right = tree2;

			tree = treeBig;
		}
	}
	return tree;
}

CMD *makeCMD(token **list)
{
	CMD *tree2 = makeSequence(list);

	if(error == ERROR || tree2 == NULL)
	{
		return freeCMD(tree2);
	}

	if(listIndex < listLen && (list[listIndex]->type == SEP_END || list[listIndex]->type == SEP_BG))
	{
		CMD *tree;

		if(list[listIndex]->type == SEP_END)
		{
			tree = mallocCMD(SEP_END, NULL, NULL);
		}
		else
		{
			tree = mallocCMD(SEP_BG, NULL, NULL);
		}

		listIndex++;

		tree->left = tree2;
		tree->right = NULL;

		return tree;
	}
	else
	{
		return tree2;
	}
}

CMD *makeSequence(token **list)
{
	CMD *tree = makeAndOr(list);

	if(error == ERROR || tree == NULL)
	{
		return freeCMD(tree);
	}

	while(((tree != NULL) && listIndex < listLen) && 
		((list[listIndex]->type == SEP_END) || (list[listIndex]->type == SEP_BG)))
	{
		CMD* treeBig;

		if(list[listIndex]->type == SEP_END)
		{
			treeBig = mallocCMD(SEP_END, NULL, NULL);
		}
		else
		{
			treeBig = mallocCMD(SEP_BG, NULL, NULL);
		}

		listIndex++;

		if(listIndex >= listLen)
		{
			error = ERROR;
			fprintf(stderr, "parsley: NULL command sequence\n");

			freeCMD(tree);
			return freeCMD(treeBig);
		}	

		CMD *tree2 = makeAndOr(list);

		if(error == ERROR || tree2 == NULL)
		{
			freeCMD(tree);
			freeCMD(treeBig);
			return freeCMD(tree2);
		}
		else
		{
			treeBig->left = tree;
			treeBig->right = tree2;

			tree = treeBig;
		}
	}
	return tree;
}


