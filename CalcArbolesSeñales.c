/*----------------------------------------------
Practica 3.
Código fuente: CalcArbolesConcurrent.c
Grau Informática
18068091G Jose Miguel Avellana López.
X8592934L Yassine El Kihal.
-------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>

#include "ConvexHull.h"

#define DMaxArboles 	25
#define DMaximoCoste 999999
#define S 10000
#define DDebug 0


  //////////////////////////
 // Estructuras de datos //
//////////////////////////



// Definicin estructura arbol entrada (Conjunto �boles).
struct  Arbol
{
	int	  IdArbol;
	Point Coord;			// Posicin �bol
	int Valor;				// Valor / Coste �bol.
	int Longitud;			// Cantidad madera �bol
};
typedef struct Arbol TArbol, *PtrArbol;



// Definicin estructura Bosque entrada (Conjunto �boles).
struct Bosque
{
	int 		NumArboles;
	TArbol 	Arboles[DMaxArboles];
};
typedef struct Bosque TBosque, *PtrBosque;



// Combinacion .
struct ListaArboles
{
	int 		NumArboles;
 	float		Coste;
	float		CosteArbolesCortados;
	float		CosteArbolesRestantes;
	float		LongitudCerca;
	float		MaderaSobrante;
	int 		Arboles[DMaxArboles];
};
typedef struct ListaArboles TListaArboles, *PtrListaArboles;

struct estadisticas {
	int EstadisticasParciales;
	int EstCombinaciones, EstCombValidas, EstCombInvalidas;
	long EstCosteTotal;
	int EstMejorCosteCombinacion, EstPeorCosteCombinacion, EstMejorArboles, EstMejorArbolesCombinacion, EstPeorArboles, EstPeorArbolesCombinacion;
	float EstMejorCoste, EstPeorCoste;
};

//Parametros de la funcion CalcularConvinacionOptima
struct args {
    int arg1;
    int arg2;
	int* arg3;
    int* arg4;
	struct estadisticas* arg5;
};

// Vector est�ico Coordenadas.
typedef Point TVectorCoordenadas[DMaxArboles], *PtrVectorCoordenadas;


typedef enum {false, true} bool;


  ////////////////////////
 // Variables Globales //
////////////////////////

TBosque ArbolesEntrada;



  //////////////////////////
 // Prototipos funciones //
//////////////////////////

bool LeerFicheroEntrada(char *PathFicIn);
bool GenerarFicheroSalida(TListaArboles optimo, char *PathFicOut);
bool CalcularCercaOptima(PtrListaArboles Optimo);
void OrdenarArboles();
void* CalcularCombinacionOptima(void *argumentos);
int EvaluarCombinacionListaArboles(int Combinacion, void *estParcial, void *estGlobal);
int ConvertirCombinacionToArboles(int Combinacion, PtrListaArboles CombinacionArboles);
int ConvertirCombinacionToArbolesTalados(int Combinacion, PtrListaArboles CombinacionArbolesTalados);
void ObtenerListaCoordenadasArboles(TListaArboles CombinacionArboles, TVectorCoordenadas Coordenadas);
float CalcularLongitudCerca(TVectorCoordenadas CoordenadasCerca, int SizeCerca);
float CalcularDistancia(int x1, int y1, int x2, int y2);
int CalcularMaderaArbolesTalados(TListaArboles CombinacionArboles);
int CalcularCosteCombinacion(TListaArboles CombinacionArboles);
void MostrarArboles(TListaArboles CombinacionArboles);

void MostrarEstadisticas(void *estPuntero);

int numHilos;
pthread_barrier_t barr;
pthread_cond_t Sincro;
pthread_mutex_t mutexSincro;
int nsincro = 0;
pthread_mutex_t mutexMejorComb;
sem_t semaforo;
int M=25000;

int main(int argc, char *argv[])
{
	TListaArboles Optimo;

	printf("Introduce numero de hilos:   ");
	scanf("%d", &numHilos);
	
	if (argc<2 || argc>4)
		printf("Error Argumentos. Usage: CalcArboles <Fichero_Entrada> [<Fichero_Salida>]");

	if (!LeerFicheroEntrada(argv[1]))
	{
		printf("Error lectura fichero entrada.\n");
		exit(1);
	}
	if (argc==4)
	{
		char *m=argv[3];
		M=atoi(m);
	}

	if (!CalcularCercaOptima(&Optimo))
	{
		printf("Error CalcularCercaOptima.\n");
		exit(1);
	}

	if (argc==2)
	{
		if (!GenerarFicheroSalida(Optimo, "./Valla.res"))
		{
			printf("Error GenerarFicheroSalida.\n");
			exit(1);
		}
	}
	else
	{
		if (!GenerarFicheroSalida(Optimo, argv[2]))
		{
			printf("Error GenerarFicheroSalida.\n");
			exit(1);
		}
	}
	exit(0);
}



bool LeerFicheroEntrada(char *PathFicIn)
{
	FILE *FicIn;
	int a;

	FicIn=fopen(PathFicIn,"r");
	if (FicIn==NULL)
	{
		perror("Lectura Fichero entrada.");
		return false;
	}
	printf("Datos Entrada:\n");

	// Leemos el nmero de arboles del bosque de entrada.
	if (fscanf(FicIn, "%d", &(ArbolesEntrada.NumArboles))<1)
	{
		perror("Lectura arboles entrada");
		return false;
	}
	printf("\tÁrboles: %d.\n",ArbolesEntrada.NumArboles);

	// Leer atributos arboles.
	for(a=0;a<ArbolesEntrada.NumArboles;a++)
	{
		ArbolesEntrada.Arboles[a].IdArbol=a+1;
		// Leer x, y, Coste, Longitud.
		if (fscanf(FicIn, "%d %d %d %d",&(ArbolesEntrada.Arboles[a].Coord.x), &(ArbolesEntrada.Arboles[a].Coord.y), &(ArbolesEntrada.Arboles[a].Valor), &(ArbolesEntrada.Arboles[a].Longitud))<4)
		{
			perror("Lectura datos arbol.");
			return false;
		}
		printf("\tÁrbol %d-> (%d,%d) Coste:%d, Long:%d.\n", a+1, ArbolesEntrada.Arboles[a].Coord.x, ArbolesEntrada.Arboles[a].Coord.y, ArbolesEntrada.Arboles[a].Valor, ArbolesEntrada.Arboles[a].Longitud);
	}

	return true;
}



bool GenerarFicheroSalida(TListaArboles Optimo, char *PathFicOut)
{
	FILE *FicOut;
	int a;

	FicOut=fopen(PathFicOut,"w+");
	if (FicOut==NULL)
	{
		perror("Escritura fichero salida.");
		return false;
	}

	// Escribir arboles a talartalado.
	 // Escribimos nmero de arboles a talar.
	if (fprintf(FicOut, "Cortar %d árbol/es: ", Optimo.NumArboles)<1)
	{
		perror("Escribir nmero de arboles a talar");
		return false;
	}


	for(a=0;a<Optimo.NumArboles;a++)
	{
		// Escribir nmero arbol.
		if (fprintf(FicOut, "%d ", ArbolesEntrada.Arboles[Optimo.Arboles[a]].IdArbol)<1)
		{
			perror("Escritura nmero �bol.");
			return false;
		}
	}

	// Escribimos coste arboles a talar.
	if (fprintf(FicOut, "\nMadera Sobrante: \t%4.2f (%4.2f)", Optimo.MaderaSobrante,  Optimo.LongitudCerca)<1)
	{
		perror("Escribir coste arboles a talar.");
		return false;
	}

	// Escribimos coste arboles a talar.
	if (fprintf(FicOut, "\nValor árboles cortados: \t%4.2f.", Optimo.CosteArbolesCortados)<1)
	{
		perror("Escribir coste arboles a talar.");
		return false;
	}

		// Escribimos coste arboles a talar.
	if (fprintf(FicOut, "\nValor árboles restantes: \t%4.2f\n", Optimo.CosteArbolesRestantes)<1)
	{
		perror("Escribir coste arboles a talar.");
		return false;
	}

	return true;

}


bool CalcularCercaOptima(PtrListaArboles Optimo)
{
	int MaxCombinaciones = (int) pow(2.0,ArbolesEntrada.NumArboles);

	OrdenarArboles();

	Optimo->Coste = DMaximoCoste;  //Optimo contiene la combinacion optima global.

    pthread_mutex_init(&mutexMejorComb, NULL);
	sem_init(&semaforo, 0 ,1);

    pthread_mutex_init(&mutexSincro, NULL);
	pthread_cond_init(&Sincro, NULL);

	if(MaxCombinaciones-((MaxCombinaciones/numHilos)+1)>=M || numHilos==1){
		if(pthread_barrier_init(&barr, NULL, numHilos)){
        	printf(" No se puede crear la barrera.\n");
        	return -1;
    	}
	}else{
		if(pthread_barrier_init(&barr, NULL, numHilos-1)){
        	printf(" No se puede crear la barrera.\n");
        	return -1;
    	}
	}
	

	struct estadisticas *estGlobal = (struct estadisticas *)malloc(sizeof(struct estadisticas)); 
		estGlobal->EstCombinaciones=1;
		estGlobal->EstCombValidas=0;
		estGlobal->EstCombInvalidas=0; 
		estGlobal->EstCosteTotal=0;
		estGlobal->EstMejorCosteCombinacion=DMaximoCoste;
		estGlobal->EstPeorCosteCombinacion=0;
		estGlobal->EstMejorArboles=ArbolesEntrada.NumArboles;
		estGlobal->EstPeorArboles=0;
		estGlobal->EstMejorArbolesCombinacion=0;
		estGlobal->EstPeorArbolesCombinacion=0;
		estGlobal->EstMejorCoste=DMaximoCoste;
		estGlobal->EstPeorCoste=0;


	int i=0; int j=0;  		//Sirven como contadores, la i para controlar el inicio y el final de cada bloque, y la j para contar el hilo.
	pthread_t hilos[numHilos];

	int bloque;
    int Combinacion, MejorCombinacion=0, CosteMejorCombinacion=DMaximoCoste;
	int Coste;
    TListaArboles OptimoParcial;

	TListaArboles CombinacionArboles;
	TVectorCoordenadas CoordArboles, CercaArboles;
	int NumArboles, PuntosCerca;
	float MaderaArbolesTalados;

	while (i<MaxCombinaciones){

		bloque=(MaxCombinaciones-i)/(numHilos-j);
		struct args *Argumentos = (struct args *)malloc(sizeof(struct args));   //Reserva memoria para los atributos del metodo que ejecutara el hilo.
			Argumentos->arg1=i+1;  //Primera combinacion del bloque.
			Argumentos->arg2=bloque+i;	//Ultima combinacion del bloque.
			Argumentos->arg3=&MejorCombinacion;  
            Argumentos->arg4=&CosteMejorCombinacion;
			Argumentos->arg5=estGlobal;
//printf("PUES YO VOY A CREAR EL HILO\n");
		if(pthread_create(&hilos[j], NULL, CalcularCombinacionOptima, (void *)Argumentos)){
			printf("Error al crear hilo\n");
			exit(1);
		}
		j++; i=bloque+i; //Se aumenta j y se iguala i 
	}

    // Punto de sincronización
    pthread_mutex_lock(&mutexSincro);
	while(nsincro<numHilos){
		pthread_cond_wait(&Sincro, &mutexSincro);
	}
	pthread_mutex_unlock(&mutexSincro);

    pthread_cond_destroy(&Sincro);
	pthread_barrier_destroy(&barr);

	pthread_mutex_destroy(&mutexSincro);
    pthread_mutex_destroy(&mutexMejorComb);

	sem_destroy(&semaforo);

	printf("\n\n\nEstadisticas Globales Final\n");
	MostrarEstadisticas(estGlobal);
	free(estGlobal);
    
	ConvertirCombinacionToArbolesTalados(MejorCombinacion, &OptimoParcial);
	printf("Optimo %d-> Coste %d, %d Arboles talados: \n", MejorCombinacion, CosteMejorCombinacion, OptimoParcial.NumArboles);
	MostrarArboles(OptimoParcial);
	printf("\n");


	// Asignar combinacin encontrada.
	ConvertirCombinacionToArbolesTalados(MejorCombinacion, Optimo);
	Optimo->Coste = CosteMejorCombinacion;
	// Calcular estadisticas óptimo.
	NumArboles = ConvertirCombinacionToArboles(MejorCombinacion, &CombinacionArboles);
	ObtenerListaCoordenadasArboles(CombinacionArboles, CoordArboles);
	PuntosCerca = chainHull_2D( CoordArboles, NumArboles, CercaArboles );

	Optimo->LongitudCerca = CalcularLongitudCerca(CercaArboles, PuntosCerca);
	MaderaArbolesTalados = CalcularMaderaArbolesTalados(*Optimo);
	Optimo->MaderaSobrante = MaderaArbolesTalados - Optimo->LongitudCerca;
	Optimo->CosteArbolesCortados = CosteMejorCombinacion;
	Optimo->CosteArbolesRestantes = CalcularCosteCombinacion(CombinacionArboles);

	return true;
}


void OrdenarArboles()
{
  int a,b;
  
	for(a=0; a<(ArbolesEntrada.NumArboles-1); a++)
	{
		for(b=1; b<ArbolesEntrada.NumArboles; b++)
		{
			if ( ArbolesEntrada.Arboles[b].Coord.x < ArbolesEntrada.Arboles[a].Coord.x ||
				 (ArbolesEntrada.Arboles[b].Coord.x == ArbolesEntrada.Arboles[a].Coord.x && ArbolesEntrada.Arboles[b].Coord.y < ArbolesEntrada.Arboles[a].Coord.y) )
			{
				TArbol aux;

				// aux=a
				aux.Coord.x = ArbolesEntrada.Arboles[a].Coord.x;
				aux.Coord.y = ArbolesEntrada.Arboles[a].Coord.y;
				aux.IdArbol = ArbolesEntrada.Arboles[a].IdArbol;
				aux.Valor = ArbolesEntrada.Arboles[a].Valor;
				aux.Longitud = ArbolesEntrada.Arboles[a].Longitud;

				// a=b
				ArbolesEntrada.Arboles[a].Coord.x = ArbolesEntrada.Arboles[b].Coord.x;
				ArbolesEntrada.Arboles[a].Coord.y = ArbolesEntrada.Arboles[b].Coord.y;
				ArbolesEntrada.Arboles[a].IdArbol = ArbolesEntrada.Arboles[b].IdArbol;
				ArbolesEntrada.Arboles[a].Valor = ArbolesEntrada.Arboles[b].Valor;
				ArbolesEntrada.Arboles[a].Longitud = ArbolesEntrada.Arboles[b].Longitud;

				// b=aux
				ArbolesEntrada.Arboles[b].Coord.x = aux.Coord.x;
				ArbolesEntrada.Arboles[b].Coord.y = aux.Coord.y;
				ArbolesEntrada.Arboles[b].IdArbol = aux.IdArbol;
				ArbolesEntrada.Arboles[b].Valor = aux.Valor;
				ArbolesEntrada.Arboles[b].Longitud = aux.Longitud;
			}
		}
	}
}


// Calcula la combinacin ptima entre el rango de combinaciones PrimeraCombinacion-UltimaCombinacion.

 void* CalcularCombinacionOptima(void *input)
{
	int PrimeraCombinacion = ((struct args*)input)->arg1; //Se almacena el contenido de la estructura de argumentos en variables locales.
	int UltimaCombinacion = ((struct args*)input)->arg2;
	int *MejorCombinacion = ((struct args*)input)->arg3;
    int *CosteMejorCombinacion = ((struct args*)input)->arg4;
	struct estadisticas *estGlobal = ((struct args*)input)->arg5;
	free(input);  //Despues de almacenar las variables, ya se puede liberar memoria.
    int MejorCombinacionAhora; //Para poder liberar antes el mutex.
    int CosteMejorCombinacionAhora;
	TListaArboles OptimoParcial;

	// Inicializar Estadisticas
	struct estadisticas *estParcial = (struct estadisticas *)malloc(sizeof(struct estadisticas)); 
		estParcial->EstCombinaciones=1;
		estParcial->EstCombValidas=0;
		estParcial->EstCombInvalidas=0; 
		estParcial->EstCosteTotal=0;
		estParcial->EstMejorCosteCombinacion=DMaximoCoste;
		estParcial->EstPeorCosteCombinacion=0;
		estParcial->EstMejorArboles=ArbolesEntrada.NumArboles;
		estParcial->EstPeorArboles=0;
		estParcial->EstMejorArbolesCombinacion=0;
		estParcial->EstPeorArbolesCombinacion=0;
		estParcial->EstMejorCoste=DMaximoCoste;
		estParcial->EstPeorCoste=0;
	int m=0;

	for (int Combinacion=PrimeraCombinacion; Combinacion<=UltimaCombinacion; Combinacion++) //Recorremos todas las combinaciones, contando tambien la ultima.
	{
	//printf("\tC%d -> \t",Combinacion);
		int Coste = EvaluarCombinacionListaArboles(Combinacion, (void *)estParcial, (void *)estGlobal);  //Se almacena el coste de la combinacion.
 	//printf("             %d\n",Coste);
        pthread_mutex_lock(&mutexMejorComb);
		if ( Coste < *CosteMejorCombinacion)
		{
			*CosteMejorCombinacion = Coste;
			*MejorCombinacion = Combinacion;
		}
        CosteMejorCombinacionAhora=*CosteMejorCombinacion;
        MejorCombinacionAhora=*MejorCombinacion;
        pthread_mutex_unlock(&mutexMejorComb);
		if ((Combinacion%S)==0)
		{
			ConvertirCombinacionToArbolesTalados(MejorCombinacionAhora, &OptimoParcial);
			printf("\r[%d] OptimoParcial %d-> Coste %d, %d Arboles talados:", Combinacion, MejorCombinacionAhora, CosteMejorCombinacionAhora, OptimoParcial.NumArboles);
			MostrarArboles(OptimoParcial);
		}
		m++;
		if (m==M){
			printf("\n\n\nEstadisticas Parciales y Global M\n");
			MostrarEstadisticas((void *)estParcial);
			sem_wait(&semaforo);
			MostrarEstadisticas((void *)estGlobal);  
			sem_post(&semaforo);
			// Punto de sincronización M
			int rc = pthread_barrier_wait(&barr);
			if(rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD){
				printf("No puedo esperar en la barrera m.\n");
				exit(-1);
    		}	 
		}
	}

	printf("\n");



    // Punto de sincronización final.
	pthread_mutex_lock(&mutexSincro);
	nsincro++;
	pthread_cond_signal(&Sincro);
	pthread_mutex_unlock(&mutexSincro);

	printf("\n\n\nEstadisticas Parciales Final\n");
	MostrarEstadisticas((void *)estParcial);
	free(estParcial);
	
	pthread_exit(NULL);  //Termina el hilo.
}


void MostrarEstadisticas(void *estPuntero)
{
	printf("\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	printf("++ Eval Comb: %d \tValidas: %d \tInvalidas: %d\tCoste Validas: %.3f\n", ((struct estadisticas*)estPuntero)->EstCombinaciones, ((struct estadisticas*)estPuntero)->EstCombValidas, ((struct estadisticas*)estPuntero)->EstCombInvalidas, (float) (((struct estadisticas*)estPuntero)->EstCosteTotal) /(float) (((struct estadisticas*)estPuntero)->EstCombValidas) );
	printf("++ Mejor Comb (coste): %d Coste: %.3f  \tPeor Comb (coste): %d Coste: %.3f\n",((struct estadisticas*)estPuntero)->EstMejorCosteCombinacion, ((struct estadisticas*)estPuntero)->EstMejorCoste, ((struct estadisticas*)estPuntero)->EstPeorCosteCombinacion, ((struct estadisticas*)estPuntero)->EstPeorCoste);
	printf("++ Mejor Comb (árboles): %d Arboles: %d  \tPeor Comb (árboles): %d Arboles %d\n",((struct estadisticas*)estPuntero)->EstMejorArbolesCombinacion, ((struct estadisticas*)estPuntero)->EstMejorArboles, ((struct estadisticas*)estPuntero)->EstPeorArbolesCombinacion, ((struct estadisticas*)estPuntero)->EstPeorArboles);
	printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
}


int EvaluarCombinacionListaArboles(int Combinacion, void *estParcial, void *estGlobal)
{
	TVectorCoordenadas CoordArboles, CercaArboles;
	TListaArboles CombinacionArboles, CombinacionArbolesTalados;
	int NumArboles, NumArbolesTalados, PuntosCerca, CosteCombinacion;
	float LongitudCerca, MaderaArbolesTalados;

	// Convertimos la combinacin al vector de arboles no talados.
	NumArboles = ConvertirCombinacionToArboles(Combinacion, &CombinacionArboles);

	// Obtener el vector de coordenadas de arboles no talados.
	ObtenerListaCoordenadasArboles(CombinacionArboles, CoordArboles);

	// Calcular la cerca
	PuntosCerca = chainHull_2D( CoordArboles, NumArboles, CercaArboles );

	/* Evaluar si obtenemos suficientes �boles para construir la cerca */
	LongitudCerca = CalcularLongitudCerca(CercaArboles, PuntosCerca);

	// Evaluar la madera obtenida mediante los arboles talados.
	// Convertimos la combinacin al vector de arboles no talados.
	NumArbolesTalados = ConvertirCombinacionToArbolesTalados(Combinacion, &CombinacionArbolesTalados);
	if (DDebug) printf(" %d arboles cortados: ",NumArbolesTalados);
	if (DDebug) MostrarArboles(CombinacionArbolesTalados);
  	MaderaArbolesTalados = CalcularMaderaArbolesTalados(CombinacionArbolesTalados);
	if (DDebug) printf("  Madera:%4.2f  \tCerca:%4.2f ",MaderaArbolesTalados, LongitudCerca);
	if (LongitudCerca > MaderaArbolesTalados || MaderaArbolesTalados==0 || NumArbolesTalados==0 || NumArboles==0)
	{
		((struct estadisticas*)estParcial)->EstCombInvalidas++;
		sem_wait(&semaforo);
		((struct estadisticas*)estGlobal)->EstCombInvalidas++;
		sem_post(&semaforo);
		// Los arboles cortados no tienen suficiente madera para construir la cerca.
	if (DDebug) printf("\tCoste:%d",DMaximoCoste);
    return DMaximoCoste;
	}

	// Evaluar el coste de los arboles talados.
	CosteCombinacion = CalcularCosteCombinacion(CombinacionArbolesTalados);

	// Estadisticas Combinaciones Parcial.
	((struct estadisticas*)estParcial)->EstCombValidas++;
	((struct estadisticas*)estParcial)->EstCosteTotal+=CosteCombinacion;
	if (CosteCombinacion<((struct estadisticas*)estParcial)->EstMejorCoste)
	{
		((struct estadisticas*)estParcial)->EstMejorCoste = CosteCombinacion;
		((struct estadisticas*)estParcial)->EstMejorCosteCombinacion =  Combinacion;
	}
	else if (CosteCombinacion>((struct estadisticas*)estParcial)->EstPeorCoste)
	{
		((struct estadisticas*)estParcial)->EstPeorCoste = CosteCombinacion;
		((struct estadisticas*)estParcial)->EstPeorCosteCombinacion =  Combinacion;
	}
	if (NumArboles<((struct estadisticas*)estParcial)->EstMejorArboles)
	{
		((struct estadisticas*)estParcial)->EstMejorArboles = NumArboles;
		((struct estadisticas*)estParcial)->EstMejorArbolesCombinacion =  Combinacion;
	}
	else if (NumArboles>((struct estadisticas*)estParcial)->EstPeorArboles)
	{
		((struct estadisticas*)estParcial)->EstPeorArboles = NumArboles;
		((struct estadisticas*)estParcial)->EstPeorArbolesCombinacion =  Combinacion;
	}

	sem_wait(&semaforo);
	// Estadisticas Combinaciones Global.
	((struct estadisticas*)estGlobal)->EstCombValidas++;
	((struct estadisticas*)estGlobal)->EstCosteTotal+=CosteCombinacion;
	if (CosteCombinacion<((struct estadisticas*)estGlobal)->EstMejorCoste)
	{
		((struct estadisticas*)estGlobal)->EstMejorCoste = CosteCombinacion;
		((struct estadisticas*)estGlobal)->EstMejorCosteCombinacion =  Combinacion;
	}
	else if (CosteCombinacion>((struct estadisticas*)estGlobal)->EstPeorCoste)
	{
		((struct estadisticas*)estGlobal)->EstPeorCoste = CosteCombinacion;
		((struct estadisticas*)estGlobal)->EstPeorCosteCombinacion =  Combinacion;
	}
	if (NumArboles<((struct estadisticas*)estGlobal)->EstMejorArboles)
	{
		((struct estadisticas*)estGlobal)->EstMejorArboles = NumArboles;
		((struct estadisticas*)estGlobal)->EstMejorArbolesCombinacion =  Combinacion;
	}
	else if (NumArboles>((struct estadisticas*)estGlobal)->EstPeorArboles)
	{
		((struct estadisticas*)estGlobal)->EstPeorArboles = NumArboles;
		((struct estadisticas*)estGlobal)->EstPeorArbolesCombinacion =  Combinacion;
	}
	sem_post(&semaforo);

	if (DDebug) printf("\tCoste:%d",CosteCombinacion);
  
	return CosteCombinacion;
}


int ConvertirCombinacionToArboles(int Combinacion, PtrListaArboles CombinacionArboles)
{
	int arbol=0;

	CombinacionArboles->NumArboles=0;
	CombinacionArboles->Coste=0;

	while (arbol<ArbolesEntrada.NumArboles)
	{
		if ((Combinacion%2)==0)
		{
			CombinacionArboles->Arboles[CombinacionArboles->NumArboles]=arbol;
			CombinacionArboles->NumArboles++;
			CombinacionArboles->Coste+= ArbolesEntrada.Arboles[arbol].Valor;
		}
		arbol++;
		Combinacion = Combinacion>>1;
	}

	return CombinacionArboles->NumArboles;
}


int ConvertirCombinacionToArbolesTalados(int Combinacion, PtrListaArboles CombinacionArbolesTalados)
{
	int arbol=0;

	CombinacionArbolesTalados->NumArboles=0;
	CombinacionArbolesTalados->Coste=0;

	while (arbol<ArbolesEntrada.NumArboles)
	{
		if ((Combinacion%2)==1)
		{
			CombinacionArbolesTalados->Arboles[CombinacionArbolesTalados->NumArboles]=arbol;
			CombinacionArbolesTalados->NumArboles++;
			CombinacionArbolesTalados->Coste+= ArbolesEntrada.Arboles[arbol].Valor;
		}
		arbol++;
		Combinacion = Combinacion>>1;
	}

	return CombinacionArbolesTalados->NumArboles;
}



void ObtenerListaCoordenadasArboles(TListaArboles CombinacionArboles, TVectorCoordenadas Coordenadas)
{
	int c, arbol;

	for (c=0;c<CombinacionArboles.NumArboles;c++)
	{
    arbol=CombinacionArboles.Arboles[c];
		Coordenadas[c].x = ArbolesEntrada.Arboles[arbol].Coord.x;
		Coordenadas[c].y = ArbolesEntrada.Arboles[arbol].Coord.y;
	}
}


	
float CalcularLongitudCerca(TVectorCoordenadas CoordenadasCerca, int SizeCerca)
{
	int x;
	float coste;
	
	for (x=0;x<(SizeCerca-1);x++)
	{
		coste+= CalcularDistancia(CoordenadasCerca[x].x, CoordenadasCerca[x].y, CoordenadasCerca[x+1].x, CoordenadasCerca[x+1].y);
	}
	
	return coste;
}



float CalcularDistancia(int x1, int y1, int x2, int y2)
{
	return(sqrt(pow((double)abs(x2-x1),2.0)+pow((double)abs(y2-y1),2.0)));
}



int 
CalcularMaderaArbolesTalados(TListaArboles CombinacionArboles)
{
	int a;
	int LongitudTotal=0;
	
	for (a=0;a<CombinacionArboles.NumArboles;a++)
	{
		LongitudTotal += ArbolesEntrada.Arboles[CombinacionArboles.Arboles[a]].Longitud;
	}
	
	return(LongitudTotal);
}



int 
CalcularCosteCombinacion(TListaArboles CombinacionArboles)
{
	int a;
	int CosteTotal=0;
	
	for (a=0;a<CombinacionArboles.NumArboles;a++)
	{
		CosteTotal += ArbolesEntrada.Arboles[CombinacionArboles.Arboles[a]].Valor;
	}
	
	return(CosteTotal);
}




void
MostrarArboles(TListaArboles CombinacionArboles)
{
	int a;

	for (a=0;a<CombinacionArboles.NumArboles;a++)
		printf("%d ",ArbolesEntrada.Arboles[CombinacionArboles.Arboles[a]].IdArbol);

  for (;a<ArbolesEntrada.NumArboles;a++)
    printf("  ");
}
	