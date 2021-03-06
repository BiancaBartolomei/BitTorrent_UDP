#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define SizeBuffer 1024
#define TEMPORIZADOR 1

int opcao = 0;
int n_arg, sock, tempo_limite, timer, recebeu, porta_servidor_rastreador, porta_servidor, server_socket;
FILE *arquivo_entrada;
char nome_arquivo[200], buffer_resposta[2], ip_servidor[15];
pthread_t thread_temporizador, thread_reconhecimento;
struct sockaddr_in serv_addr, cli_addr;
struct sockaddr_in remoteServAddr;


//==============================================================================
//Função para validar checksum
//==============================================================================
int valida_checksum(char buffer[], int tamanho){

  int i;
  char check=0;
  for(i=0 ; i < tamanho -2 ; i++){

    check += buffer[i];
    check &= 127;
  }
  printf("\n====== Verificação do Checksum ======\n");
  printf("Checksum esperado: 0x%x\n",buffer[tamanho-1]);
  printf("Checksum real: 0x%x\n",check);

  if(buffer[tamanho-1] == (check)){
      printf("Status do pacote: OK!\n");
      return 1;
  }

  else{
      printf("Status do pacote: ERROR\n");
      return 0;
  }
  printf("\n=====================================\n");
}

//==============================================================================
//Função para gerar checksum
//==============================================================================
char checksum(char data[], int tamanho){

  int i;
  char checksum = 0;
  for(i = 0 ; i<tamanho; i++){

    checksum += data[i];
    checksum &= 127;
  }
  return checksum;
}


//==============================================================================
//Função de espera do temporizador
//==============================================================================
void *funcao_temporizador(){
    sleep(TEMPORIZADOR);
    tempo_limite = 1;
    recebeu = 0;
    timer = 0;
}

//==============================================================================
//Função para cancelar thread
//==============================================================================
int pthread_cancel(pthread_t thread);


//==============================================================================
//Função para espera de reconhecimento
//==============================================================================
void *funcao_reconhecimento(){
  ssize_t resposta;
  int addr_len = sizeof(remoteServAddr);
  resposta = recvfrom(server_socket, &buffer_resposta, 2, 0,(struct sockaddr *) &remoteServAddr, &addr_len);
  recebeu = 1;
  timer = 0;
}

//==============================================================================
//Leitura do arquivo
//==============================================================================
void ler_arquivo(char nome_arquivo[]){

	arquivo_entrada = fopen(nome_arquivo, "rb");
	if(!arquivo_entrada){
		printf("ERROR: O arquivo não existe\n");
	}
}

//==============================================================================
//Escrita do arquivo
//==============================================================================
void escrever_arquivo(char nome_arquivo[]){
  char src[10] = "_1";
  strcat(nome_arquivo, src);

	arquivo_entrada = fopen(nome_arquivo, "wb");
	if(!arquivo_entrada){
		printf("ERROR: O arquivo não existe\n");
	}

}

//==============================================================================
//Função de Cliente
//==============================================================================
void * client_function(){

  //Inicia a função de cliente
  scanf("%d", &opcao);
  if(opcao == 1){
    int client_socket, bytes_recebidos, binder;
  	ssize_t ler_bytes, escrever_bytes, resposta;
  	struct sockaddr_in serv_addr;
    struct hostent *h;
    char buffer_entrada[SizeBuffer + 3], buffer_resposta[2], buffer_rastreador[4];
    char numero_pacote = 0;

    //Conexao com o servidor rastreador
  	if(n_arg < 2){
  		printf("Uso correto: endereco IP - porta\n");
  		exit(1);
  	}

    h = gethostbyname(ip_servidor);
    if(h==NULL) {
      printf("ERROR: Nome do host\n");
      exit(1);
    }

    remoteServAddr.sin_family = h->h_addrtype;
    memcpy((char *) &remoteServAddr.sin_addr.s_addr,
     h->h_addr_list[0], h->h_length);
    remoteServAddr.sin_port = htons(3030);


    //Criação do socket
    client_socket = socket(AF_INET,SOCK_DGRAM,0);
    if(client_socket<0) {
      printf("ERROR: Socket não pode ser aberto \n");
      exit(1);
    }


    // Conexão com qualquer porta
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(0);


    binder = bind(client_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if(binder<0) {
      printf("ERROR: Não é possivel conectar a porta\n");
      exit(1);
    }


    //Requisita arquivo ao servidor rastreador
  	printf("Insira o nome do arquivo a ser requisitado:\n");
    scanf("%s", nome_arquivo);

    resposta = sendto(client_socket, nome_arquivo, sizeof(nome_arquivo), 0,(struct sockaddr *) &remoteServAddr, sizeof(remoteServAddr));

    int addr_len = sizeof(remoteServAddr);
    bytes_recebidos = recvfrom(client_socket, &buffer_rastreador, 4, 0,(struct sockaddr *) &remoteServAddr, &addr_len);
    close(client_socket);

    //Verifica a resposta do servidor rastreador
    if(!strcmp(buffer_rastreador,"0000")){
      printf("ERROR: Arquivo não encontrado\n");
      return 0;
    }

    /////////////////////////////////////////////////////////////
    //Conexao com outro cliente torrent
    h = gethostbyname(ip_servidor);
    if(h==NULL) {
      printf("ERROR: Nome do host\n");
      exit(1);
    }

      remoteServAddr.sin_family = h->h_addrtype;
      memcpy((char *) &remoteServAddr.sin_addr.s_addr,
       h->h_addr_list[0], h->h_length);
      remoteServAddr.sin_port = htons(atoi(buffer_rastreador));

      //Criando socket
      client_socket = socket(AF_INET,SOCK_DGRAM,0);
      if(client_socket<0) {
        printf("ERROR: Socket não pode ser aberto \n");
        exit(1);
      }


      /* bind any port */
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
      serv_addr.sin_port = htons(0);


      binder = bind(client_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
      if(binder<0) {
        printf("ERROR: Não é possivel conectar com a porta\n");
        exit(1);
      }

      addr_len = sizeof(remoteServAddr);

      escrever_bytes = sendto(client_socket, nome_arquivo, sizeof(nome_arquivo), 0,(struct sockaddr *) &remoteServAddr, sizeof(remoteServAddr));
      //escrever_bytes = write(client_socket, nome_arquivo, sizeof(nome_arquivo));
			if(escrever_bytes == 0){
				printf("Erro em manda o nome do arquivo\n");
				close(client_socket);
			}

			escrever_arquivo(nome_arquivo);

      //While para transferencia dos pacotes
			while(1){

        //Recebe pacote no buffer_entrada
				bytes_recebidos = recvfrom(client_socket, &buffer_entrada, SizeBuffer+3, 0,(struct sockaddr *) &remoteServAddr, &addr_len);
				fwrite(&buffer_entrada,sizeof(char),bytes_recebidos-3,arquivo_entrada);

        //Verifica o numero de sequencia
        if(numero_pacote > buffer_entrada[bytes_recebidos-1]){
          printf("Pacote duplicado detectado\n");
          printf("Envio de reconhecimento ACK\n");
          buffer_resposta[0] = 1;
          buffer_resposta[1] = numero_pacote;
          resposta = sendto(client_socket, buffer_resposta, 2, 0,(struct sockaddr *) &remoteServAddr, sizeof(remoteServAddr));
          continue;
        }

          //Valida checksum do pacote
          if(!valida_checksum(buffer_entrada,bytes_recebidos-1)){
            printf("Numero do NACK = %d\n",numero_pacote);

            buffer_resposta[0] = 2;
            buffer_resposta[1] = numero_pacote;
            resposta = sendto(client_socket, buffer_resposta, 2, 0,(struct sockaddr *) &remoteServAddr, sizeof(remoteServAddr));
  					continue;
  				} else {
            printf("Numero do ACK = %d\n",numero_pacote);
            buffer_resposta[0] = 1;
            buffer_resposta[1] = numero_pacote;
            resposta = sendto(client_socket, buffer_resposta, 2, 0,(struct sockaddr *) &remoteServAddr, sizeof(remoteServAddr));
          }


        numero_pacote++;
        numero_pacote %= 128;

        //Fecha o arquivo no caso de ser o ultimo pacote
				if(buffer_entrada[bytes_recebidos-3] == '1'){
					fclose(arquivo_entrada);
					break;
				}
			}
  	close(client_socket);
  }

  return 0;
}
//==============================================================================
//Função de Servidor
//==============================================================================
void * server_function(){
  int  binder, bytes_enviados,bytes_restantes, rc;
	int transferencia_completa = 0, quantidade_bytes_enviados = 0, numero_pacotes_enviados = 0;
  ssize_t ler_bytes, escrever_bytes, resposta;
  socklen_t clilen;
	char buffer_envio[SizeBuffer+3];
	char contador_pacote = 0;

  //Conexao com o cliente
	if(n_arg < 2){
		printf("Uso correto: endereco IP - porta\n");
		exit(1);
	}

	server_socket = socket(AF_INET, SOCK_DGRAM, 0);

	if(server_socket <= 0){
		printf("Erro na abertura do socket: %s\n", strerror(errno));
		exit(1);
	}
	else if(server_socket){
		printf("=== Cliente torrent em espera ===\n");
    printf("Digite '1' para buscar um arquivo.\n");
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(porta_servidor);

	binder = bind(server_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if(binder < 0){
		printf("Erro no Bind: %s\n", strerror(errno));
		exit(1);
	}


	clilen = sizeof(cli_addr);


  ler_bytes = recvfrom(server_socket, &nome_arquivo, sizeof(nome_arquivo), 0,(struct sockaddr*) &cli_addr, &clilen);

  ler_arquivo(nome_arquivo);

  //Verifica o tamnho do arquivo
  fseek(arquivo_entrada,0,SEEK_END);
  long long tamanho_arquivo = ftell(arquivo_entrada);
  printf("%lld\n", tamanho_arquivo);
  fseek(arquivo_entrada,0,SEEK_SET);

  //While para transferencia dos pacotes
	while(1){

		if(transferencia_completa) break;

    //If para pacotes de tamnho padrão
		if(quantidade_bytes_enviados + SizeBuffer < tamanho_arquivo){

      //Cria e envia um pacote com dados, flag de pacote final, checksum e numero de sequencia
			fread(&buffer_envio, SizeBuffer, 1, arquivo_entrada);
			buffer_envio[SizeBuffer] = '0';
			buffer_envio[SizeBuffer+1] = checksum(buffer_envio, SizeBuffer);
			buffer_envio[SizeBuffer+2] = contador_pacote;

			bytes_enviados = sendto(server_socket, buffer_envio, SizeBuffer+3, 0,(struct sockaddr *) &cli_addr, sizeof(cli_addr));

			if(bytes_enviados<0) {
		    printf("ERROR: Não foi possivel enviar os dados\n");
		    close(server_socket);
		    exit(1);
		  }

      //Inicio do temporizador
      timer = 1;
      recebeu = 0;
      tempo_limite = 0;
      pthread_create( &thread_temporizador, NULL, funcao_temporizador, NULL);
      pthread_create( &thread_reconhecimento, NULL, funcao_reconhecimento, NULL);


      while(timer){
          //Espera pelo estouro do temporizador ou pacote de reconhecimento
      }

      pthread_cancel(thread_reconhecimento);
      pthread_cancel(thread_temporizador);

      //Verifica o reconhecimento do pacote
      if(buffer_resposta[0] == 1 && recebeu == 1){
        printf("Numero de sequencia do ACK: %d\n", buffer_resposta[1]);
        contador_pacote++;
        contador_pacote %= 128;
      } else {
        if(tempo_limite){
          printf("\nTempo de espera excedido\n");
        } else {
          printf("Numero de sequencia do NACK: %d\n",buffer_resposta[1] );
        }
        bytes_enviados = sendto(server_socket, buffer_envio, SizeBuffer+3, 0,(struct sockaddr *) &cli_addr, sizeof(cli_addr));
      }

			numero_pacotes_enviados++;
			quantidade_bytes_enviados += bytes_enviados;

			memset(buffer_envio,0x0, SizeBuffer);

		}
    //Else para paotes com tamnho menor que o padrão
    else {

			bytes_restantes = tamanho_arquivo - quantidade_bytes_enviados;
			memset(buffer_envio,0x0,SizeBuffer);

      //Cria e envia um pacote com dados, flag de pacote final, checksum e numero de sequencia
			fread(&buffer_envio, SizeBuffer, 1, arquivo_entrada);
			buffer_envio[bytes_restantes] = '1';
			buffer_envio[bytes_restantes+1] = checksum(buffer_envio, bytes_restantes);
			buffer_envio[bytes_restantes+2] = contador_pacote;

			bytes_enviados = sendto(server_socket, buffer_envio, bytes_restantes+3, 0,(struct sockaddr *) &cli_addr, sizeof(cli_addr));

			if(bytes_enviados<0) {
		    printf("ERROR: Não foi possivel enviar os dados\n");
		    close(server_socket);
		    exit(1);
		  }

      //Inicio do temporizador
      timer = 1;
      recebeu = 0;
      tempo_limite = 0;

      pthread_create( &thread_temporizador, NULL, funcao_temporizador, NULL);
      pthread_create( &thread_reconhecimento, NULL, funcao_reconhecimento, NULL);


      while(timer){
          //Espera pelo estouro do temporizador ou pacote de reconhecimento
      }

      pthread_cancel(thread_reconhecimento);
      pthread_cancel(thread_temporizador);

      //Verifica o reconhecimento do pacote
      if(buffer_resposta[0] == 1 && recebeu == 1){
        printf("Numero de sequencia do ACK:: %d\n", buffer_resposta[1]);
        contador_pacote++;
        contador_pacote %= 128;
      } else {
        if(tempo_limite){
          printf("\nTempo de espera excedido\n");
        } else {
          printf("Numero de sequencia do NACK: %d\n",buffer_resposta[1] );
        }
        bytes_enviados = sendto(server_socket, buffer_envio, bytes_restantes+3, 0,(struct sockaddr *) &cli_addr, sizeof(cli_addr));
      }

			numero_pacotes_enviados++;
			quantidade_bytes_enviados += bytes_enviados;

			memset(buffer_envio,0x0, SizeBuffer);
			transferencia_completa = 1;
		}
	}

	close(sock);
	close(server_socket);
  return 0;
}
//==============================================================================
//FUNÇÃO PRINCIPAL
//==============================================================================
int main(int argc, char const *argv[]) {
  pthread_t server, client;

  //Definição das portas para conexao
  n_arg = argc;
  strcpy(ip_servidor,argv[1]);
  porta_servidor_rastreador = 3030;
  porta_servidor = atoi(argv[2]);

  //Inicia as thread de servidor e cliente torrent
  pthread_create(&server, NULL, server_function,NULL);
  pthread_create(&client, NULL, client_function,NULL);

  pthread_join(client, NULL);
  pthread_join(server, NULL);


  return 0;
}
