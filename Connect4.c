/*Abhinav Singhal
 *ICSI 333
 *Connect Four Part 3
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h> 
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h> 

/* global variable declaration */
int width = 7, height = 6;
char player1[32], player2[32], worldStatus[256];
char** board;
int playerTurn, isServer = 1;
char flag = 'T', sym1 = '1', sym2 = '2';
int tcpsocket, clientsocket;
char enemyInput;

/* function declaration */
void initialization(int argc, char *argv[]);
void teardown();
char acceptInput(char check);
void update(char input);
void display();
int connect4(char player);
void playerCharSetter();

/*
 * Function: main
 * --------------
 * Plays the game called Connect 4. Contains the game loop.
 *
 * return: zero if game ran successfully.
 */
int main(int argc, char *argv[]) {
	if(argc < 2){
		printf("No port given. Ending Program...\n");
		return 1;
	}
	initialization(argc, argv);
	display();
	//For Loop that calls all three functions.
	for(int i = 1; flag != 'F'; i++){
		if(i%2 == 1) playerTurn = 1;
		else playerTurn = 2; // Used to decide which player's turn it is.
		if(playerTurn == isServer){
			update(acceptInput('F')); 
		}
		else{
			printf("Other players move, Please Wait.\n");
			char buf[2];
			int num = recv(clientsocket, (void*)buf, 2, 0);
			enemyInput = buf[0];
			update(enemyInput); //update local board with input from other player.
		}
		display();
	}
	teardown();
	return 0; 
}

/* 
 * Function: initialization
 * ------------------------
 * Asks for names of players and allocates memory to the board.
 *
 * argc: number of arguments given to main
 * argv: array that contains ip and port
 */
void initialization(int argc, char *argv[]){
	printf("Setting Up The Game\n");
	
	struct addrinfo hints, *info, *cur;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM; //TCP socket
	
	if(argc > 2){
		printf("Starting in Client Mode:\n");
		isServer = 2;
		getaddrinfo(argv[1], argv[2], &hints, &info);
		for(cur = info; cur!=NULL; cur = cur->ai_next){
			clientsocket = socket(cur->ai_family, cur->ai_socktype, 0);
			if(clientsocket < 0){
				perror("socket failure");
				exit(1);
			}
			if(connect(clientsocket, cur->ai_addr, cur->ai_addrlen) < 0){
				perror("connection failure");
				close(clientsocket);
				exit(1);
			}
			break;
		}
		if(cur == NULL) exit(1); //cur failed
		
		freeaddrinfo(info);
		
		printf("Enter your name: ");
		scanf("%31s", player2); //save client name as player 2
		while ((getchar()) != '\n');
		printf("Player 2 is %s \n", player2);
		send(clientsocket, player2, 32, 0); //send name to server
		int count = recv(clientsocket, (void*)player1, 32, 0);
		printf("Player 1 is %s \n", player1); //print player 1 name
	}
	else {
		printf("Starting in Server Mode:\n");
		hints.ai_flags = AI_PASSIVE; //get IP address
		
		getaddrinfo(NULL, argv[1], &hints, &info); //use given port
		for(cur = info; cur!=NULL; cur = cur->ai_next){
			tcpsocket = socket(cur->ai_family, cur->ai_socktype, 0);
			if(tcpsocket < 0){
				perror("socket failure");
				exit(1);
			}
			if(bind(tcpsocket, cur->ai_addr, cur->ai_addrlen) < 0){
				perror("binding failure");
				close(tcpsocket);
				exit(1);
			}
			break;
		}
		if(cur == NULL) exit(1);
		
		char address[1000];
		struct sockaddr_in *in = (struct sockaddr_in *)cur->ai_addr;
		//convert ip address to string value
		inet_ntop(AF_INET, &(in->sin_addr), address, 1000);
		printf("The IP Address is: %s\n", address);
		printf("The port is: %s\n", argv[1]);
		
		listen(tcpsocket,1); //wait for client to respond
		
		struct sockaddr_storage client;
		
		int size = sizeof(client);
		
		clientsocket = accept(tcpsocket, (struct sockaddr*)&client, &size);
		
		freeaddrinfo(info);
		
		printf("Enter enter your name: ");
		scanf("%31s", player1); //%31s makes sure name doesn't surpass allocated space.
		while ((getchar()) != '\n'); //clears input buffer.
		printf("Player 1 is %s \n", player1);
		int count = recv(clientsocket, (void*)player2, 32, 0);
		send(clientsocket, player1, 32, 0); //send player name to client
		printf("Player 2 is %s \n", player2);
		
	}
	
	//dynamically allocate memory for the board.
	board = (char**) malloc(height*sizeof(char*));
	for(int i = 0; i<height; i++) board[i] = (char *)malloc(width * sizeof(char));
	//fill board
	for(int i = 0; i<height; i++)
		for(int j = 0; j < width; j++) board[i][j] = '0';
	
	
	//set character for each player.
	playerCharSetter();
}

/* 
 * Function: teardown
 * ------------------
 * Frees memory allocated to the board.
 */
void teardown(){
	printf("Destroying the Game.\n");
	
	//free memory
	for(int i = 0; i < height; i++) free(board[i]);
	free(board);
	
	//free sockets
	close(tcpsocket);
	close(clientsocket);
}

/* 
 * Function: acceptInput
 * ---------------------
 * Takes input from the player.
 * 
 * returns: The value of the input.
 */
char acceptInput(char check){
	char input;
	int goodInput = 1; //Used for error checking.
	int numinput; //input in ASCII
	if(check == 'F'){
		if(playerTurn == 1) {
			printf("\033[0;31m"); //changes text color.
			printf("%s's Turn\n", player1);
		}
		else {
			printf("\033[0;33m");
			printf("%s's Turn\n", player2);
		}
		printf("\033[0;0m"); //resets text color to default.
	}
	while(goodInput == 1){
		if(check == 'T') printf("Column full pick another: ");
		else printf("Which column? Valid letters are A-G(7 columns) and H to Quit: ");
		scanf(" %c", &input);
		numinput = (int)(input);
		//error trap if user inputs value outside accepted range.
		if(numinput < 73 && numinput > 64) goodInput = 0; 
		else{
			printf("Bad input not within range try again.\n");
			goodInput = 1;
		}
	}
	return input;
}

/* 
 * Function: update
 * ----------------
 * Updates the world state.
 *
 * input: The input passed from the acceptInput Function.
 */
void update(char input){
	
	int rawinput = (int)input - 65; //convert letter to column number.
	int i, boardLimit = 0;
	char winText[64] = " is the Winner!";
	char endText[128];
	
	//drop disk in chosen column.
	//uses the sym global variable which stands for symbol
	//and represents the character displayed for each player.
	for(i = 5; i >= 0; i--){
		if(board[i][rawinput] == '0'){
			if(playerTurn == 1) board[i][rawinput] = sym1;
			else board[i][rawinput] = sym2;
			i = -2;
		}
	}
	
	//if column is full ask player to select different column.
	if(i == -1){
		//unless they are qutting
		if(rawinput != 7){
			update(acceptInput('T'));
		}
	}
	
	//turnholder sends input to other player
	if(playerTurn == isServer){
		char buf[2];
		buf[0] = input;
		send(clientsocket, buf, 2, 0);	
	}
		
	//goes through board to check if its full.
	for(i = 0; i < height; i++){
		for(int j = 0; j < width; j++){
			if(board[i][j] != '0') boardLimit++;
		}
	}
	//End game if board is full.
	if(boardLimit == 42){
		strcpy(worldStatus, "All columns are full! Game Ends!");
		flag = 'F';
	}
	
	//if player chose to quit game then end game.
	if(input == 'H'){
		char giveup[64] = " quits the game!";
		if(playerTurn == 1){
			strcpy(endText, player1);
			strcat(endText, giveup);
			strcpy(worldStatus, endText);
		}
		else{
			strcpy(endText, player2);
			strcat(endText, giveup);
			strcpy(worldStatus, endText);
		}
		flag = 'F';
	}
	
	//check for win condition using connect4 function.
	if(playerTurn == 1){
		if(connect4(sym1) == 1){
			strcpy(endText, player1);
			strcat(endText, winText);
			strcpy(worldStatus, endText);
			flag = 'F';
		}
	}
	else{
		if(connect4(sym2) == 1){
			strcpy(endText, player2);
			strcat(endText, winText);
			strcpy(worldStatus, endText);
			flag = 'F';
		}
	}
}

/* 
 * Function: display
 * -----------------
 * Prints the current state of the world.
 */
void display(){
	
	printf("\033[0;34m");//color code to blue
	printf("---------------\n");
	for(int i = 0; i < height; i++){
		for(int j = 0; j < width; j++){
			if(board[i][j] == '0') printf("\033[0;34m");//blue
			else if(board[i][j] == sym1) printf("\033[0;31m");//red
			else printf("\033[0;33m");//yellow
			printf(" %c", board[i][j]);
		}
		printf("\n");
	}
	printf("\033[0;34m");//blue
	printf("---------------\n");
	printf("\033[1;35m");//bold magenata
	
	printf("%s \n", worldStatus);
	printf("\033[0;0m");//reset to default.
}

/*
 * Function: connect4
 * ------------------
 * Checks to see if a win condition is met.
 * 
 * player: The character that belongs to the current turnholder.
 *
 * returns: int value, one if win condition met.
 */
int connect4(char player){
	
	//horizontal Check -
	for(int j = 0; j < width-3; j++){
		for(int i = 0; i < height; i++){
			if(board[i][j] == player &&
				board[i][j+1] == player &&
				board[i][j+2] == player &&
				board[i][j+3] == player) return 1; 
		}
	}
	
	//vertical Check |
	for(int i = 0; i < height - 3; i++){
		for(int j = 0; j<width; j++){
			if(board[i][j] == player &&
				board[i+1][j] == player &&
				board[i+2][j] == player &&
				board[i+3][j] == player) return 1;
		}
	}
	
	//ascendingDiagonal Check /
	for(int i = 3; i < height; i++){
		for(int j = 0; j < width-3; j++){
			if(board[i][j] == player &&
				board[i-1][j+1] == player &&
				board[i-2][j+2] == player &&
				board[i-3][j+3] == player) return 1;
		}
	}
	
	//descendingDiagonal Check
	for(int i = 3; i < height; i++){
		for(int j = 3; j < width; j++){
			if(board[i][j] == player &&
				board[i-1][j-1] == player &&
				board[i-2][j-2] == player &&
				board[i-3][j-3] == player) return 1;
		}
	}
	return 0;
}

/*
 * Function: playerCharSetter
 * --------------------------
 * Sets the character that is displayed on the board for each player.
 */
void playerCharSetter(){
	//uses the first letter of the name as the character.
	//shuffles through each letter of name if the letter is '0' or '2'.
	for(int i = 0; i < strlen(player1); i++){
		if(player1[i] != '0' && player1[i] != '2'){
			sym1 = player1[i];
			i = 33; //end loop
		}
	}
	for(int i = 0; i < strlen(player2); i++){
		//Checks to make sure both players character isn't the same.
		if(player2[i] != '0' && player2[i] != sym1){
			sym2 = player2[i];
			i = 33; //end loop
		}
	}
}