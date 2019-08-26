
#include <ilcp/cp.h>

ILOSTLBEGIN

#include <fstream>
#include <string>
#include <vector>
#include <iostream>


unsigned int ILOSC_KLOCKOW = 6;

int wymiarX = 12, wymiarY = 12;

struct Klocek{
	Klocek(const IloEnv &env,unsigned int w, unsigned int h):
		w(w),o(env,0,1),h(h),tablica(env)
	{
		for(int x =0; x<wymiarX; x++)
		{
			tablica.add(IloIntVarArray(env));
			for(int y=0;y<wymiarY;y++)
			{
				tablica[x].add(IloIntVar(env,0,1));
			}
		}
	}

	IloInt w;//szerokoœæ produktu 
	IloInt h;//wysokoœæ produktu 
	IloBoolVar o; //zmienna decyzyjny orientacji o wartoœciach ze zbiouru {0,1} (bez obrotu, obrót)

	// tablica zmiennych decyzyjnych obrazujaca palete i polozenie na niej obecnego klocka (o ile zosta³ wybrany)
	IloArray<IloIntVarArray> tablica;
};


// wczytanie danych z pliku
bool wczytanieDanych(const IloEnv& env, const string nazwa_pliku, std::vector<Klocek> &ref)
{
	fstream fInputFile;
	fInputFile.open(nazwa_pliku);
	if(fInputFile.good())
	{
		std::string sLine;
		std::getline(fInputFile,sLine);
		std::stringstream stream(sLine);
		stream>>wymiarX>>wymiarY>>ILOSC_KLOCKOW;

		if(wymiarX<1||wymiarY<1||ILOSC_KLOCKOW<1)
		{
			cout<<"nieprawidlowy rozmiar palety albo ilosc klockow"<<endl;
			return false;
		}
		for(int i=0; i<ILOSC_KLOCKOW; i++)
		{
			int a=0,b=0;
			std::getline(fInputFile,sLine);
			std::stringstream stream(sLine);
			stream>>a>>b;
			if(a>wymiarX || a<1 || b>wymiarY || b<1)
			{
				cout<<"bledne dane!!!!"<<endl;
				return false;
			}

			Klocek k(env,a,b);
			ref.push_back(k);
		}
		return true;
	}
	else
	{
		cout<<"nieprawidlowy plik!"<<endl;
		return false;
	}
}

// obrazowanie wyniku w pliku tekstowym oraz zapisanie do pliku komend matlaba pozwalaj¹cych na zobrazowanie u³ozenia klockow na palecie
void rysowanieWPliku(const IloCP &cplex, const vector<Klocek> & wszystkieKlocki)
{
	std::vector<std::vector<char> > tablica;
	for(int i=0;i<wymiarX;i++)
	{
		tablica.emplace_back();
		for(int j=0;j<wymiarY;j++)
		{
			tablica.at(i).push_back('\0');
		}
	}
	char znaki[] ={'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','0','1','2','3','4','5','6','7','8','9'};

	fstream MatlabFile;
	MatlabFile.open("Matlab.txt",'w');

	if(MatlabFile.good())
	{
		MatlabFile<<"plot(";
	}
	for(int i = 0; i<ILOSC_KLOCKOW; i++)
	{
		int x1=wymiarX;
		int x2=0;
		int y1=wymiarY;
		int y2=0;
		bool klocekWystepuje=false;
		for(int x=0;x<wymiarX;x++)
		{
			for(int y=0;y<wymiarY;y++)
			{
				if(cplex.getValue(wszystkieKlocki[i].tablica[x][y])==1)
				{
					klocekWystepuje=true;
					tablica[x][y]= znaki[i];
					if(x1>x)
					{
						x1=x;
					}
					if(y1>y)
					{
						y1=y;
					}
					if(x2<x+1)
					{
						x2=x+1;
					}
					if(y2<y+1)
					{
						y2=y+1;
					}
				}
			}
		}
		if(MatlabFile.good()&&klocekWystepuje)
		{
			//	'k-',[0 6 6 0 0],[0 0 6 6 0],'k-',[6 12 12 0 0],[0 0 6 6 0],'k-',[12 18 18 12 12],[0 0 6 6 0],'k-'
			MatlabFile<<"["<<x1<<" "<<x2<<" "<<x2
				<<" "<<x1<<" "<<x1<<"],["<<y1<<" "<<y1
				<<" "<<y2
				<<" "<<y2<<" "<<y1<<"],'k-',";
		}
	}

	fstream outputFile;
	outputFile.open("wynik.txt",'w');
	if(outputFile.good())
	{
		for(int y=wymiarY -1;y>=0;y--)
		{
			for(int x =0; x<wymiarX;x++)
			{
				outputFile<<tablica[x][y];
			}
			outputFile<<endl;
		}
	}
	else
	{
		cout<<"problem z zapisem wyniku do pliku"<<endl;
	}
}

// g³ówna funkcja programu
int main()
{
	IloEnv env;

	std::vector<Klocek> wszystkieKlocki;
	if(true == wczytanieDanych(env, "dane.txt",wszystkieKlocki))
	{
		try
		{
			IloModel model(env);

			// Wybór wariantu problemu do rozwi¹zania
			int Wariant = 1;
			cout<<"Nr problemu do rozwiazania:"<<endl;
			cout<<"(1) Maksymalizacja pola powierzchni (domyslne)"<<endl;
			cout<<"(2) Ukladanie elementow do rogu palety. Funkcja celu x+y->min"<<endl;
			cout<<"(3) Ukladanie elementow do rogu palety. Funkcja celu x*y->min"<<endl;
			cout<<":";
			cin>>Wariant;

			//domyslna wartosc
			if (Wariant>3 || Wariant<1)
			{
				Wariant = 1;
			} 
			cout<<endl<<endl;

			IloIntVar x_max(env, 0, wymiarX);
			IloIntVar y_max(env, 0, wymiarY);

			// dodanie ograniczen
			for(int x = 0; x<wymiarX; x++)
			{
				for(int y=0;y<wymiarY;y++)
				{
					for(int i = 0;i <ILOSC_KLOCKOW; i++)
					{
						for(int j = i+1 ; j<ILOSC_KLOCKOW; j++)
						{
							model.add(wszystkieKlocki[i].tablica[x][y]+wszystkieKlocki[j].tablica[x][y] <=1 );//warunek ¿eby klocki na siebie nie nachodzi³y
						}
					}
				}
			}
			IloIntExpr poleMax(env);
			for(int i=0;i<ILOSC_KLOCKOW;i++)
			{
				IloIntExpr sumaCalosci(env);
				for(int x =0;x<wymiarX;x++)
				{
					IloIntExpr suma1(env);
					for(int y =0; y<wymiarY;y++)
					{
						suma1+=wszystkieKlocki[i].tablica[x][y];//suma w kolumnie
						sumaCalosci+=wszystkieKlocki[i].tablica[x][y];//suma calosci
						poleMax+=wszystkieKlocki[i].tablica[x][y];//pole do maksymalizacji
					}
					model.add(suma1==0||suma1 ==wszystkieKlocki[i].h +wszystkieKlocki[i].w*wszystkieKlocki[i].o - wszystkieKlocki[i].h*wszystkieKlocki[i].o);// ilosc pól równa siê wysokoœæ klocka lub zero
				}
				if(1 == Wariant)
				{
					model.add(sumaCalosci==wszystkieKlocki[i].w*wszystkieKlocki[i].h|| sumaCalosci==0 );//warunek ¿eby ka¿dy klocek zosta³ dok³adnie raz u¿yty lub nie zosta³ u¿yty w cale
				}
				else
				{
					model.add(sumaCalosci==wszystkieKlocki[i].w*wszystkieKlocki[i].h);//warunek ¿eby ka¿dy klocek zosta³ dok³adnie raz u¿yty
				}
				// suma w wierszu
				for(int y =0; y<wymiarY;y++)
				{
					IloIntExpr suma2(env);
					for(int x =0;x<wymiarX;x++)
					{
						suma2+=wszystkieKlocki[i].tablica[x][y];
					}
					model.add(suma2==0||suma2 ==  wszystkieKlocki[i].w +wszystkieKlocki[i].h*wszystkieKlocki[i].o - wszystkieKlocki[i].w*wszystkieKlocki[i].o);
				}
				//warunki ¿eby klocki nie by³y rozstrzelone
				for(int x=0;x<wymiarX;x++)
				{
					for(int y=0;y<wymiarY;y++)
					{
						for(int k = x+wszystkieKlocki[i].w; k<wymiarX; k++)
						{
							model.add(wszystkieKlocki[i].tablica[x][y]*wszystkieKlocki[i].tablica[k][y]*(1-wszystkieKlocki[i].o) == 0);
						}
						for(int k = x+wszystkieKlocki[i].h; k<wymiarX; k++)
						{
							model.add(wszystkieKlocki[i].tablica[x][y]*wszystkieKlocki[i].tablica[k][y]*wszystkieKlocki[i].o == 0);
						}

						for(int k = y+wszystkieKlocki[i].h; k<wymiarY; k++)
						{
							model.add(wszystkieKlocki[i].tablica[x][y]*wszystkieKlocki[i].tablica[x][k]*(1- wszystkieKlocki[i].o) == 0);
						} 
						for(int k = y+wszystkieKlocki[i].w; k<wymiarY; k++)
						{
							model.add(wszystkieKlocki[i].tablica[x][y]*wszystkieKlocki[i].tablica[x][k]*wszystkieKlocki[i].o == 0);
						} 
					}
				}
			}

			//dodanie do modelu funkcji celu
			if(1 == Wariant)
			{
				model.add(IloMaximize(env,poleMax));
			}
			else
			{
				for(int i = 0;i<ILOSC_KLOCKOW;i++)
				{
					for(int y=0;y<wymiarY;y++)
					{
						IloIntVarArray tabX(env);

						for(int x =0; x<wymiarX;x++)
						{
							tabX.add(wszystkieKlocki[i].tablica[x][y]);
						}
						tabX.add(IloIntVar(env,0,0));

						IloIntVar indexX(env, 0,wymiarX-1);
						model.add(((1==IloElement(tabX,indexX)) && (0==IloElement(tabX,indexX+1))) == (IloSum(tabX)>0));//
						model.add(indexX<=x_max);
					}

					for(int x =0; x<wymiarX;x++)
					{
						IloIntVarArray tabY(env);
						for(int y=0;y<wymiarY;y++)
						{
							tabY.add(wszystkieKlocki[i].tablica[x][y]);
						}
						tabY.add(IloIntVar(env,0,0));

						IloIntVar indexY(env, 0,wymiarY-1);
						model.add(((1==IloElement(tabY,indexY)) && (0==IloElement(tabY,indexY+1))) == (IloSum(tabY)>0));
						model.add(indexY<=y_max);
					}

				}
				if (2 == Wariant)
				{
					model.add(IloMinimize(env, x_max+ y_max ));
				}
				else
				{
					model.add(IloMinimize(env, x_max* y_max ));
				}
			}
			
			
			IloCP cp(model);

			// ustawienie limitu czasowego
			cp.setParameter(IloCP::NumParam::TimeLimit,3600);

			// uruchomienie solvera
			if(cp.solve())
			{
				rysowanieWPliku(cp, wszystkieKlocki);
			}
			else{
				cout<<"brak rozwi¹zania"<<endl;
			}
		}
		catch (IloException& ex) {
			cerr << "Error: " << ex << endl;
		}
		catch (...) {
			cerr << "Error" << endl;
		}
	}


	env.end();

	getchar();//stop 
	return 0;
}