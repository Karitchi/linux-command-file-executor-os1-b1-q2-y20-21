#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
//http://manpagesfr.free.fr/man/man2/wait.2.html
//Src :https://www.ibm.com/docs/en/ztpf/1.1.0.15?topic=apis-waitpidobtain-status-information-from-child-process
int main(int argc, char ** argv)
{	
	for(int fileArg = 1; fileArg < argc ; fileArg++)
	{		
		int fileOpened = open(argv[fileArg], O_RDONLY);

		if(fileOpened < 0)
		{
			printf("\e[0;31mA problem occured while trying to access : %s : %s ! \n", argv[fileArg], strerror(errno));
			continue;//passe au fichier suivant si le précédent ne peut pas être ouvert			
		}
		else
		{
			if(fileArg == 1)
				printf("\e[0;35mWelcome to shell executer !\n");

			printf("\n\e[0;35mStarting executing command(s) from file : %s", argv[fileArg]);
		}

		struct stat status;//struct pour récupèrer la taille du fichier

		if(stat(argv[fileArg], &status) == 0)
		{
			int fileBytes;
			
			char *bufferFile = (char*)malloc(sizeof(char*) *  status.st_size); //préparation d'un buffer pour récupérer le contenu du fichier

			fileBytes = read(fileOpened ,&bufferFile[0], status.st_size);//lecture du fichier -> remplissage du buffer

			if(fileBytes < 0)
			{
				printf("\e[0;31mCannot read file %s ! \n" , argv[fileArg]);
				continue;//passe au fichier suivant si celui-ci ne peut pas être ouvert , peu importe la raison 
			}
			else
			{
				printf("\nBytes read : %d \e[0;34m\n" , fileBytes);
			}

			close(fileOpened);//ferme le file descriptor une fois le fichier lu

			if(status.st_size == 0)//vérifie si il y a bien du contenu dans le fichier
			{
				printf("\e[0;31m\nFollowing file does not contain any command : %s" ,  argv[fileArg]);
				printf("\n");
				continue;
			}

   			char *subtoken, *saveptr1, *token;//préparation des pointeurs pour la double division (ligne + espace) et l'appel système strtok_r

   			int nbArgs;

        	for ( ; ; bufferFile = NULL)//boucle sur les lignes 
        	{	
            	token = strtok_r(bufferFile, "\n", &saveptr1);//sauve la ligne à laquelle il est dans token
          
        		if (token == NULL || *token == '\n') 
        		{
                	break;//plus de lignes à casser
            	}

            	char **arg = (char**)malloc(sizeof(char*));//prépartion du tableau d'arguments par ligne

            	char *saveptr2;

            	for(int j = 0;; j++,token = NULL)//boucle sur les espaces
            	{
                	subtoken = strtok_r(token," ",&saveptr2);//sauve l'argument dans subtoken
                	arg[j] = subtoken;//met dans le tableau l'argument récupéré 
                	arg = (char**)realloc(arg, sizeof(char*) * (j + 2));//agrandit le tableau pour les autres argumments

                	if (subtoken == NULL)
                	{                     		
                		nbArgs = j;              	        	
                    	break;//plus d'espace à casser
                	}
            	}

            	int returned;

        		pid_t childProc;

        		int pipefd[2], chld_state;

    			pipe(pipefd);//ouvre le pipe

    			childProc = fork();//création du processus fils
			
        		if(childProc == 0)
        		{

					close(pipefd[0]);//fermeture du pipe en lecture

        			int err1 = dup2(pipefd[1],1);//duplication entre le stdout et le pipe en écriture

        			int err2 = execvp(arg[0], arg);//exécution de la commande et ses arguments

        			//continue si le execvp a une erreur et print les erreurs ,sinon sort du fils
        			/*
        			for(int k = 0; k < nbArgs; k++)
					{
						switch(k)
						{
							case 0:
								printf( "\e[0;31mCannot execute command : %s ", arg[k]);
								break;

							case 1:
								printf( "with parameter(s) : %s ", arg[k]);
								break;

							default:
								printf( "%s ", arg[k]);
								
						}					
					}

					printf(": %s !\n", strerror(errno));//affiche l'erreur

					printf("\e[0;34m");*/
		
					exit(errno);	
        		}
        		else
        		{
        		
    				wait(&chld_state);//Attend le fils
					if (WIFEXITED(chld_state))//Vérifie si le fils a fini
					{
      					returned = WEXITSTATUS(chld_state);

      					if(chld_state == 0)//Vérifie comment le fils s'est terminé (si 0 : le fils s'est terminé sans erreur, sinon les erreurs sont vérifiées dans le fils)
      					{
		        			for(int k = 0; k < nbArgs; k++)
							{
								switch(k)
								{
									case 0:
										printf( "\e[0;32m\nSuccessful executed command : \e[0;35m%s\e[0;32m ", arg[k]);
										break;

									case 1:
										printf( "with parameter(s) : \e[0;35m%s \e[0;32m", arg[k]);
										break;

									default:
										printf( "\e[0;35m%s ", arg[k]);
										
								}						
							}

							printf("\e[0;34m \n");
      					}
      					else
      					{
      						for(int k = 0; k < nbArgs; k++)
							{
								switch(k)
								{
									case 0:
										printf( "\e[0;31m\nCannot execute command : %s ", arg[k]);
										break;

									case 1:
										printf( "with parameter(s) : %s ", arg[k]);
										break;

									default:
										printf( "%s ", arg[k]);
								
								}					
							}

							printf(": %s !\n", strerror(returned));//affiche l'erreur

							//printf("\e[0;34m");
      					}
      				
					}				

  					printf("\e[0;34m \n");

        			char readBuffer[20];//prépatation d'un buffer pour lire dans le pipe

					int nbBytes;

					close(pipefd[1]);//ferme le pipe en écriture

					//int totalBytes = 0;

					while((nbBytes = read(pipefd[0] ,readBuffer, sizeof(readBuffer))) > 0)//boucle et affiche le contenu du pipe à l'aide du buffer
					{	
						//totalBytes += nbBytes;	
						
						write(1, readBuffer, nbBytes);
					}
        		}
        		
        		free(arg);//libère le pointeur arg      		       		
    		}

    		free(token);//libère le pointeur token 

    		free(subtoken);	//libère le pointeur subtoken 

    		free(bufferFile);//libère le pointeur bufferFile 
		}		
	}
	return 0;
}