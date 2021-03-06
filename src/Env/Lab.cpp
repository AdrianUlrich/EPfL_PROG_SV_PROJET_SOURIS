#include "Lab.hpp"
#include <exception>
#include <algorithm>
#include <Application.hpp>
#include "Organ.hpp"
#include "Types.hpp"
#include <Utility/Vec2d.hpp>

//DEBUG:
#include <iostream>
using namespace std;

Lab::Lab()
	:	tracked(nullptr)
	,	cross(new sf::Sprite(buildSprite(Vec2d(0,0),40,getAppTexture(getAppConfig().entity_texture_tracked))))
	,	currentSubstance(SubstanceId::GLUCOSE)
	,	deltaGlucose(0)
	,	deltaBromo(0)
	,	deltaVGEF(0)
{
	makeBoxes(getAppConfig().simulation_lab_nb_boxes);
}

void Lab::makeBoxes(unsigned int nbCagesPerRow)
{
	if (!(boites.empty()))
		destroyBoxes();
	if (nbCagesPerRow < 1)
		throw std::invalid_argument("Cannot have zero or less cages");
	double largeur((getApp().getLabSize().x)/nbCagesPerRow);
	double hauteur((getApp().getLabSize().y)/nbCagesPerRow);
	for (unsigned int i(0); i<nbCagesPerRow; ++i)
	{
		boites.push_back(vector<Box*>(0));
		for (unsigned int j(0); j<nbCagesPerRow; ++j)
		{
			boites[i].push_back(new Box({(i+0.5)*largeur,(j+0.5)*hauteur},largeur,hauteur,largeur*0.025));
		}
	}
}

void Lab::destroyBoxes()
{
	for (auto& vec : boites)
	{
		for (auto& val : vec)
		{
			delete val;
			val=nullptr;
		}
	}
	boites.clear();
}

Lab::~Lab()
{
	delete cross;
	reset();
	destroyBoxes();
}

void Lab::update(sf::Time dt)
{
	size_t n(NTTs.size());
	for (size_t i(0); i<n; ++i)
	{//if (NTTs[i] != nullptr) // NTTs ne contient jamais de nullptr ou dangling
		NTTs[i]->update(dt);
		if (NTTs[i]->isDead())
		{	
			if (tracked==NTTs[i]) tracked=nullptr;
			
			/// letting other entities know
			for (SimulatedEntity* val : NTTs)
			{val->isDead(NTTs[i]);}
			
			size_t a(animals.size());
			for (size_t j(0); j<a; ++j)
			{if (animals[j]==NTTs[i])
			{animals[j]=animals[--a];
			animals.pop_back();}}			
			
			a = cheeses.size();
			for (size_t j(0); j<a; ++j)
			{if (cheeses[j]==NTTs[i])
			{cheeses[j]=cheeses[--a];
			cheeses.pop_back();}}
			
			/// La boite est liberee dans le destructeur de animal
			delete NTTs[i]; //NTTs[i]=nullptr;
			NTTs[i]=NTTs[--n]; //NTTs.erase(NTTs.begin()+i);
			NTTs.pop_back();
		}
	}
}

vector<SimulatedEntity*>* Lab::findTargetsInSightOf(Animal* a)
{
	vector<SimulatedEntity*>* ans(new vector<SimulatedEntity*>);
	for (auto val : NTTs)
	{//note: NTTs NEVER contain nullptrs or deleted pointers
		if
		(
			/// animal may want to see himself sometimes
			//val != a and
			/// animal may want to interact with fellow animals
			//a->eatable(val) and
			///here we give vision to the animal of everything it should see and let it deal with it
			a->isTargetInSight(val->getCenter()) //and
			/// checks for closest target but animal may decide what to do with all its sights
			//((ans==nullptr) or (distance(a->getCenter(),val->getCenter()))<(distance(a->getCenter(),ans->getCenter())))
		)
			ans->push_back(val);
	}
	return ans;
}


void Lab::drawOn(sf::RenderTarget& target)
{
	for (auto& vec : boites)
	{
		for (auto val : vec)
		{
			if (val != nullptr)
				val->drawOn(target);
		}
	}
	for (auto NTT : cheeses)
	{
		NTT->drawOn(target);
	}	
	for (auto NTT : animals)
	{
		NTT->drawOn(target);
	}
	
	if (tracked!=nullptr)
	{
		cross->setPosition(tracked->getCenter()+tracked->getRadius()*Vec2d(-1,1));
		target.draw(*cross);
	}
}

void Lab::reset()
{
	for (auto NTT : NTTs)
	{
		delete NTT;
		//NTT = nullptr;
	}
	NTTs.clear();
	animals.clear();
	cheeses.clear();
	tracked=nullptr;
}

bool Lab::addEntity(SimulatedEntity* ntt)
{
	if(ntt!=nullptr)
	{
		for(auto&vec:boites)
		{
			for(auto&val:vec)
			{
				if(ntt->canBeConfinedIn(val))
				{
					ntt->confine(val);
					NTTs.push_back(ntt);
					return true;
				}
			}
		}
		delete ntt;
	}
	return false;
}

bool Lab::addAnimal(Animal* mickey)
{
	if(addEntity(mickey))
	{
		animals.push_back(mickey);
		mickey->fillBox();
		return true;
	}
	else
		return false;
}

bool Lab::addCheese(Cheese* c)
{
	if(addEntity(c))
	{
		cheeses.push_back(c);
		return true;
	}
	else
		return false;
}


void Lab::trackAnimal(const Vec2d& p)
{
	for (auto val : animals)
	{
		if (val->isPointInside(p))
		{
			//cout<<"TAZDINGO"<<endl;
			trackAnimal(val);
			break;
		}
	}
}

void Lab::switchToView(View view)
{
	if (tracked!=nullptr)
		getApp().switchToView(view);	
}

void Lab::stopTrackingAnyEntity()
{
	tracked=nullptr;
}


void Lab::updateTrackedAnimal() 
{if(tracked!=nullptr)tracked->updateOrgan();}

void Lab::drawCurrentOrgan(sf::RenderTarget& target)
{if(tracked != nullptr)tracked->drawCurrentOrgan(target);}

void Lab::trackAnimal(Animal* n) 
{tracked=n;}

double Lab::getDelta(SubstanceId id) const
{
	switch (id)
	{
		case SubstanceId::GLUCOSE : return deltaGlucose;
		case SubstanceId::BROMOPYRUVATE : return deltaBromo;
		case SubstanceId::VGEF : return deltaVGEF;
		default: return 0;
	}
}

SubstanceId Lab::getCurrentSubst() const
{return currentSubstance;}

SubstanceId Lab::nextSubstance()
{
	switch (currentSubstance)
	{
		case SubstanceId::GLUCOSE : currentSubstance=SubstanceId::BROMOPYRUVATE;break;
		case SubstanceId::BROMOPYRUVATE : currentSubstance=SubstanceId::VGEF;break;
		case SubstanceId::VGEF : currentSubstance=SubstanceId::GLUCOSE;break;
	}
	tracked->updateCurrentSubstance(currentSubstance);
	return currentSubstance;
}

void Lab::increaseCurrentSubst()
{
	switch (currentSubstance)
	{
		case SubstanceId::GLUCOSE: deltaGlucose+=getAppConfig().delta_glucose;break;
		case SubstanceId::BROMOPYRUVATE: deltaBromo+=getAppConfig().delta_bromo;break;
		case SubstanceId::VGEF: deltaVGEF+=getAppConfig().delta_vgef;break;
	}
}

void Lab::decreaseCurrentSubst()
{
	switch (currentSubstance)
	{
		case SubstanceId::GLUCOSE: deltaGlucose-=getAppConfig().delta_glucose;break;
		case SubstanceId::BROMOPYRUVATE: deltaBromo-=getAppConfig().delta_bromo;break;
		case SubstanceId::VGEF: deltaVGEF-=getAppConfig().delta_vgef;break;
	}
}

void Lab::setCancerAt(Vec2d const& pos)
{tracked->setCancerAt(pos);}

void Lab::printSubstanceAt(Vec2d const& pos) const
{tracked->printSubstanceAt(currentSubstance,pos);}

