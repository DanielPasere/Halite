#include <stdlib.h>
#include <time.h>
#include <cstdlib>
#include <ctime>
#include <time.h>
#include <set>
#include <fstream>
#include <exception>
#include <queue>
#include "hlt.hpp"
#include "networking.hpp"

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define ABS(a) ( (a > 0) ? (a) : (-a) )
#define CONST 7
std::ofstream ceva("ceva.txt");
std::ofstream harta("map.txt");
struct Square {
	hlt::Location location;
	hlt::Site site;
};

/*
*Operatorul < a fost supraincarcat pentru a permite compararea a doua patrate
*in functie de pozitiile lor pe harta.
*/
static bool operator<(const Square& s1, const Square& s2) {
	if (s1.location.x < s2.location.x)
		return true;
	if (s1.location.x == s2.location.x)
		if (s1.location.y < s2.location.y)
			return true;
	return false;
}

/*
*Aceasta functie intoarce directia in care se doreste a fi deplasat un patrat.
*/
unsigned char direction(hlt::Location l1, hlt::Location l2,
	hlt::GameMap presentMap) {
	unsigned char dir = 0;
	unsigned short distance = presentMap.getDistance(l1, l2);
	unsigned short x1 = l1.x;
	unsigned short y1 = l1.y;
	unsigned short x2 = l2.x;
	unsigned short y2 = l2.y;


	if (2 * distance > presentMap.height + presentMap.width)
		return dir;

	if (x1 == x2) {
		if (ABS((short)(y1 - y2)) == distance) { //Daca nu trece peste margine
			if (y1 > y2)
				dir = 1;
			else
				dir = 3;
			return dir;
		}
		else {
			if (y1 > y2)
				dir = 3;
			else
				dir = 1;
			return dir;
		}
	}
	else {
		if (y1 == y2) {
			if (ABS((short)(x1 - x2)) == distance) { //Daca nu trece peste margine
				if (x1 > x2)
					dir = 4;
				else
					dir = 2;
				return dir;
			}
			else {
				if (x1 > x2)
					dir = 2;
				else
					dir = 4;
				return dir;
			}
		}
		else {
			int d1 = 2, d2 = 3;
			if (x1 > x2)
				d1 = 4;
			if (y1 > y2)
				d2 = 1;
			if (ABS((short)(x1 - x2))  > presentMap.width / 2)
				d1 = 6 - d1;
			if (ABS((short)(y1 - y2)) > presentMap.height / 2)
				d2 = 4 - d2;
			unsigned char v[2];
			v[0] = d1;
			v[1] = d2;
			return v[rand() % 2];
		}
	}
}

/*
*Aceasta functie seteaza locatia unui patrat in functie de directia aleasa
*pentru deplasare.
*/
hlt::Location setLocation(hlt::GameMap presentMap, hlt::Location location,
	unsigned char direction) {
	hlt::Location ret;
	float x, y;
	x = location.x;
	y = location.y;

	/*
	*Calculam noua locatie in functie de directia indicata
	*/
	switch (direction) {
		/*
		*Punctul de origine al hartii (0,0) este coltul din stanga sus (nord-west)
		*deci pentru a merge spre nord, y care reprezinta inaltimea trebuie sa
		*scada, iar pentru a merge spre sud, y trebuie sa fie incrementat.
		*/
	case 1:
		y--;
		break;
	case 2:
		x++;
		break;
	case 3:
		y++;
		break;
	case 4:
		x--;
	}

	/*
	*Urmatoarele 4 if-uri verifica daca patratul a trecut in marginea opusa a
	*hartii, caz in care trebuie modificate coordonatele pentru a nu primi o
	*exceptie de tip IndexOutOfBounds.
	*/
	if (x == -1)
		x = presentMap.width - 1;
	if (y == -1)
		y = presentMap.height - 1;
	if (x == presentMap.width)
		x = 0;
	if (y == presentMap.height)
		y = 0;

	ret.x = x;
	ret.y = y;
	return ret;
}

/*
*Aceasta functie este folosita pentru a seta campurile unui patrat.
*/
Square setSquare(hlt::GameMap presentMap, unsigned short b,
	unsigned short a) {
	Square square;
	square.location.x = b;
	square.location.y = a;
	square.site.owner = presentMap.contents[a][b].owner;
	square.site.production = presentMap.contents[a][b].production;
	square.site.strength = presentMap.contents[a][b].strength;
	return square;
}

/*
*Aceasta functie are rolul de a verifica daca patratul primit ca parametru
*se afla in multimea ce contine patratele aflate la frontiera.
*/
bool contains(std::set<Square> border, Square square) {
	std::set<Square>::iterator iterator;

	for (iterator = border.begin(); iterator != border.end(); iterator++)
		if (square.location.x == iterator->location.x &&
			square.location.y == iterator->location.y)
			return true;

	return false;

}

/*
*Folosim algoritmul lui Lee pentru a ne ajuta sa determinam care este vecinul
*cel mai apropiat de un anumit patrat, deoarece ne dorim sa ne deplasam spre
*acesta.
*/
void Lee(unsigned char distance[100][100], hlt::GameMap presentMap,
	std::set<Square> border, unsigned char myID) {
	std::queue< std::pair <unsigned short, unsigned short> > q;
	/*
	*Introducem intr-o coada toate patratele aflate la granita, patrate
	*reprezentate ca perechi de coordonate.
	*/
	for (std::set<Square>::iterator it = border.begin(); it != border.end();
		it++) {
		std::pair <unsigned short, unsigned short> loc;
		loc.first = it->location.x;
		loc.second = it->location.y;
		q.push(loc);
	}

	while (q.empty() == false) {
		hlt::Location loc;
		loc.x = q.front().first;
		loc.y = q.front().second;
		q.pop();
		hlt::Location neighbour;
		/*
		*Analizez cei 4 vecini ai nodului extras din coada. Daca nodul respectiv
		*nu a fost vizitat si apartine jucatorului curent, in matricea de
		*distante voi retine distanta pana la acest punct la care adaug
		*productia. Aceasta strategie este folosita pentru ocolirea patratelor
		*care au productie mare, doar in cazul in care aceasta abordare este mai
		*eficienta. Notiunea de eficiente pentru acest caz va fi detaliata in
		*README. Apoi inseram patratul vizitat inapoi in coada.
		*/
		for (int i = 1; i <= 4; i++) {
			neighbour = setLocation(presentMap, loc, i);
			if (distance[neighbour.y][neighbour.x] == 255 &&
				presentMap.contents[loc.y][loc.x].owner == myID) {
				distance[neighbour.y][neighbour.x] = distance[loc.y][loc.x] + 1 +
					presentMap.contents[loc.y][loc.x].production * 3;
				q.push(std::make_pair(neighbour.x, neighbour.y));
			}
		}
	}
}

/*
*Aceasta functie verifica daca locatia primita ca parametru se afla la frontiera
*zonei detinute de jucatorul cu ID-ul primit ca parametru.
*/
bool isBorder(hlt::GameMap presentMap, hlt::Location location,
	unsigned char myID) {
	Square northNeighbour, southNeighbour, eastNeighbour, westNeighbour;
	hlt::Location temporary;

	/*
	* Se declara si se initializeaza 4 variabile care reprezinta cei 4 vecini
	*ai locatiei curente.
	*/
	temporary = setLocation(presentMap, location, 1);
	northNeighbour = setSquare(presentMap, temporary.x, temporary.y);

	temporary = setLocation(presentMap, location, 2);
	eastNeighbour = setSquare(presentMap, temporary.x, temporary.y);

	temporary = setLocation(presentMap, location, 3);
	southNeighbour = setSquare(presentMap, temporary.x, temporary.y);

	temporary = setLocation(presentMap, location, 4);
	westNeighbour = setSquare(presentMap, temporary.x, temporary.y);

	/*
	*Daca cel putin unul din cei 4 vecini nu apartine jucatorului inseamna ca
	*locatia primita ca parametru se afla la frontiera. De asemenea, se doreste
	*ca expansiunea sa se realizeze prin cucerirea vecinilor cu productie nenula
	*pentru a fi rentabila.
	*/
	bool ok = 0;
	if (northNeighbour.site.owner != myID && northNeighbour.site.production != 0)
		ok = 1;
	if (eastNeighbour.site.owner != myID && eastNeighbour.site.production != 0)
		ok = 1;
	if (westNeighbour.site.owner != myID && westNeighbour.site.production != 0)
		ok = 1;
	if (southNeighbour.site.owner != myID && southNeighbour.site.production != 0)
		ok = 1;
	return ok;
}

double averageProduction(hlt::GameMap presentMap, hlt::Location location) {
	int i, j, x, y, totalProduction = 0;
	double averageProduction;
	for (i = -1; i <= 1; i++) {
		for (j = -1; j <= 1; j++) {

			x = location.x + i;
			y = location.y + j;
			if (x < 0)
				x += presentMap.width;
			if (x >= presentMap.width)
				x = 0;
			if (y < 0)
				y += presentMap.height;
			if (y >= presentMap.height)
				y = 0;
			
			totalProduction += presentMap.contents[y][x].production;

		}
		//
	}
	averageProduction = totalProduction / 9;
	return averageProduction;
}

void LeeOutside(float distance[100][100], hlt::GameMap presentMap,
	std::set<Square> border, unsigned char myID) {
	std::queue< std::pair <unsigned short, unsigned short> > q;
	/*
	*Introducem intr-o coada toate patratele aflate la granita, patrate
	*reprezentate ca perechi de coordonate.
	*/
	for (std::set<Square>::iterator it = border.begin(); it != border.end();
		it++) {
		std::pair <unsigned short, unsigned short> loc;
		loc.first = it->location.x;
		loc.second = it->location.y;
		q.push(loc);
	}

	while (q.empty() == false) {
		hlt::Location loc;
		loc.x = q.front().first;
		loc.y = q.front().second;
		q.pop();
		hlt::Location neighbour;
		/*
		*Analizez cei 4 vecini ai nodului extras din coada. Daca nodul respectiv
		*nu a fost vizitat si  nu apartine jucatorului curent, in matricea de
		*distante voi retine distanta pana la acest punct la care adaug
		*productia. Aceasta strategie este folosita pentru ocolirea patratelor
		*care au productie mare, doar in cazul in care aceasta abordare este mai
		*eficienta. Notiunea de eficiente pentru acest caz va fi detaliata in
		*README. Apoi inseram patratul vizitat inapoi in coada.
		*/
		for (int i = 1; i <= 4; i++) {
			neighbour = setLocation(presentMap, loc, i);
			if (distance[neighbour.y][neighbour.x] == 255 &&
				presentMap.contents[loc.y][loc.x].owner != myID) {
				distance[neighbour.y][neighbour.x] = distance[loc.y][loc.x] - 1 +
					presentMap.contents[loc.y][loc.x].production * 1.25;
				q.push(std::make_pair(neighbour.x, neighbour.y));
			}
		}
	}
}

int** createProductionMatrix(hlt::GameMap presentMap) {
	int i, j;
	int** productionMatrix = (int**)malloc(presentMap.height * sizeof(int*));
	for (i = 0; i < presentMap.width; i++)
		productionMatrix[i] = (int*)malloc(presentMap.width * sizeof(int));

	for (i = 0; i < presentMap.width; i++)
		for (j = 0; j < presentMap.height; j++) {
			hlt::Location location;
			location.x = i;
			location.y = j;
			productionMatrix[j][i] = averageProduction(presentMap, location);
		}
	return productionMatrix;
}

int main() {
	srand(time(NULL));

	std::cout.sync_with_stdio(0);

	unsigned char myID;
	hlt::GameMap presentMap;
	getInit(myID, presentMap);
	sendInit("MyBot");

	std::set<hlt::Move> moves;
	std::set<Square> border;

	int i, j;


	int** productionMatrix = createProductionMatrix(presentMap);

	for (i = 0; i < presentMap.width; i++) {
		for (j = 0; j < presentMap.height; j++) {
			ceva<< productionMatrix[j][i];
		}
		ceva << std::endl;
	}
	ceva.close();
	unsigned char distance[100][100];
	float outsideDistance[100][100];
	int turn = -1;
	while (true) {
		moves.clear();
		turn++;
		/*
		*Verificam daca in urma modificarilor generate de mutarea precedenta
		*patratele care se aflau la granita inainte si-au pastrat aceasta
		*proprietate.
		*/
		for (std::set<Square>::iterator it = border.begin(); it != border.end();
			it++) {
			if (isBorder(presentMap, it->location, myID) == false)
				border.erase(it);
		}

		/*
		*Initializam toate patratele hartii cu o valoare default 255, pentru a
		*identifica ulterior mai tarziu, care sunt patratele vizitate si care
		*apartin hartii.
		*/
		for (int i = 0; i < presentMap.height; i++)
			for (int j = 0; j < presentMap.width; j++) {
				distance[j][i] = 255;
				outsideDistance[j][i] = 255;
			}
		/*
		*Se doreste ca patratele aflate in interiorul teritoriului detinut de
		*jucator sa se deplaseze catre cea mai apropiata frontiera. Pentru aceasta,
		*folosim o matrice de distante, si retinem in ea distanta minima de la
		*locatia cu coordonatele (j,i) catre cele 4 directii spre frontiera.
		*Patratele care se afla la frontiera au distanta 0.
		*/
		for (std::set<Square>::iterator it = border.begin(); it != border.end();
			it++) {
			distance[it->location.y][it->location.x] = 0;
			outsideDistance[it->location.y][it->location.x] = 0;
		}

		/*
		*
		*/
		if (border.empty() == false) {
			Lee(distance, presentMap, border, myID);
			LeeOutside(outsideDistance, presentMap, border, myID);
		}

		for (i = 0; i < presentMap.width; i++) {
			for (j = 0; j < presentMap.height; j++) {
				if (outsideDistance[i][j] == 255)
					harta << " ";
				else
					harta << outsideDistance[i][j] << " ";
			}
			harta << std::endl;
		}
		harta << std::endl;
		harta << std::endl;
	getFrame(presentMap);

	for (unsigned short a = 0; a < presentMap.height; a++) {
		for (unsigned short b = 0; b < presentMap.width; b++) {
			if (presentMap.getSite({ b, a }).owner == myID) {
				hlt::Move move = { { b, a }, (unsigned char)(rand() % 4 + 1) };
				Square square = setSquare(presentMap, b, a);
				bool isbord = isBorder(presentMap, square.location, myID);

				/*
				*Daca patratul a ajuns in urma utimei mutari la granita si
				*nu a fost inclus in lista border, atunci il inseram acum.
				*/
				if (isbord == true && contains(border, square) == false)
					border.insert(square);

				if (isbord == true) {
					Square north, south, east, west;
					hlt::Location temporary;

					temporary = setLocation(presentMap, square.location, 1);
					north = setSquare(presentMap, temporary.x, temporary.y);

					temporary = setLocation(presentMap, square.location, 2);
					east = setSquare(presentMap, temporary.x, temporary.y);

					temporary = setLocation(presentMap, square.location, 3);
					south = setSquare(presentMap, temporary.x, temporary.y);

					temporary = setLocation(presentMap, square.location, 4);
					west = setSquare(presentMap, temporary.x, temporary.y);


					unsigned char min = 255;
					unsigned char max = 0;
					unsigned char direction = (unsigned char)0;
					/*
					*Dorim sa determinam care din vecinii ,care nu apartin
					*teritoriului jucatorului curent, au strength
					*minim si productie nenula, pentru cucerire. Cum scopul
					*etapei curente este acoperirea hartii in cel mai scurt
					*timp, dorim sa cucerim vecinul cu strength-ul cel mai
					*mic.
					*/
					if (north.site.owner != myID && north.site.production != 0)
						if (north.site.strength < min) {
							min = north.site.strength;
							direction = 1;
						}

					if (east.site.owner != myID && east.site.production != 0)
						if (east.site.strength < min) {
							min = east.site.strength;
							direction = 2;
						}

					if (south.site.owner != myID && south.site.production != 0)
						if (south.site.strength < min) {
							min = south.site.strength;
							direction = 3;
						}

					if (west.site.owner != myID && west.site.production != 0)
						if (west.site.strength < min) {
							min = west.site.strength;
							direction = 4;
						}

					/*
					*De asemenea, aici se calculeaza care dintre vecinii
					*patratului din frontiera, care apartin jucatorului,
					*are strength maxim, deoarece prin combinarea strength-ului
					*celor doua patrate sa obtinem un strength maxim.
					*/
					unsigned char direction2 = 0;
					if (north.site.owner == myID)
						if (north.site.strength > max) {
							max = north.site.strength;
							direction2 = 1;
						}
					if (east.site.owner == myID)
						if (east.site.strength > max) {
							max = east.site.strength;
							direction2 = 2;
						}
					if (south.site.owner == myID)
						if (south.site.strength > max) {
							max = south.site.strength;
							direction2 = 3;
						}
					if (west.site.owner == myID)
						if (west.site.strength > max) {
							max = west.site.strength;
							direction2 = 4;
						}
					/*
					*Daca patratul curent nu poate ataca niciun vecin inamic,
					*cu sansa de castig, atunci se combina cu patratul vecin
					*aliat care are strength maxim, doar daca strength-ul
					*patratului curent este mai mic decat acel maxim, altfel
					*mutarea este redundanta.
					*/

					if (square.site.strength <= min)
						if (square.site.strength > CONST * square.site.production &&
							square.site.strength < max)
							direction = direction2;
						else
							direction = 0;

					move.dir = direction;
				}
				else {
					/*
					*Pentru a utiliza productia patratului mai eficient, vom
					*muta un patrat doar daca strength-ul sau curent este mai
					*mare decat productia inmultita cu o constanta CONST.
					*/
					if (square.site.strength < CONST * square.site.production)
						move.dir = 0;
					else {
						unsigned short min = 255;
						unsigned char dir;
						/*
						*In cazul unui patrat interior, examinam cei 4 vecini,
						*pentru a vedea care dintre ei este cel mai apropiat
						*de frontiera.
						*/
						for (int i = 1; i <= 4; i++) {
							hlt::Location loc = setLocation(presentMap, square.location, i);
							if (distance[loc.y][loc.x] < min &&
								presentMap.contents[loc.y][loc.x].production != 0) {
								min = distance[loc.y][loc.x];
								dir = i;
							}
						}
						move.dir = dir;
					}
				}
				moves.insert(move);
			}
		}
	}
	sendFrame(moves);
}


	return 0;
}
