/***********************************************************************
**
**   windmeasurementlist.cpp
**
**   This file is part of Cumulus
**
************************************************************************
**
**   Copyright (c):  2002 by Andr� Somers
**
**   This file is distributed under the terms of the General Public
**   Licence. See the file COPYING for more information.
**
**   $Id: windmeasurementlist.cpp,v 1.10 2007/04/15 20:00:10 jwharington Exp $
**
***********************************************************************/
/*
NOTE: Some portions copyright as above

Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005  

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

}
*/

#include "stdafx.h"

#include "windmeasurementlist.h"


WindMeasurementList::WindMeasurementList(){
  nummeasurementlist = 0;
}


WindMeasurementList::~WindMeasurementList(){
  for (unsigned int i=0; i<nummeasurementlist; i++) {
    delete measurementlist[i];
  }
}


/**
  * Returns the weighted mean windvector over the stored values, or 0
  * if no valid vector could be calculated (for instance: too little or
  * too low quality data).
  */

extern int iround(double);


Vector WindMeasurementList::getWind(double Time, double alt, bool *found){
  //relative weight for each factor
  #define REL_FACTOR_QUALITY 100
  #define REL_FACTOR_ALTITUDE 100
  #define REL_FACTOR_TIME 200
#define TIME_RANGE 36 // one hour

  int altRange  = 1000; //conf->getWindAltitudeRange();
  int timeRange = TIME_RANGE*100; //conf->getWindTimeRange();

  unsigned int total_quality=0;
  unsigned int quality=0, q_quality=0, a_quality=0, t_quality=0;
  Vector result;
  WindMeasurement * m;
  int now= (int)(Time);
  double altdiff=0;
  double timediff=0;

  *found = false;

  result.x = 0;
  result.y = 0;
 
  for(uint i=0;i< nummeasurementlist; i++) {
    m= measurementlist[i];
    altdiff= (alt - m->altitude)*1.0/altRange;
    timediff= fabs((double)(now - m->time)/timeRange);

    if ((fabs(altdiff)< 1.0) && (timediff < 1.0)) {

      q_quality = m->quality* REL_FACTOR_QUALITY / 5; 
      //measurement quality
      
      a_quality = iround(((2.0/
			   (altdiff*altdiff+1.0))
			  -1.0)
			 * REL_FACTOR_ALTITUDE); 
      //factor in altitude difference between current altitude and
      //measurement.  Maximum alt difference is 1000 m.

      double k=0.0025;

      t_quality = iround(k*(1.0-timediff)/(timediff*timediff+k)
			 * REL_FACTOR_TIME);
      //factor in timedifference. Maximum difference is 1 hours.

      quality= q_quality * (a_quality * t_quality);

      result.x += m->vector.x * quality;
      result.y += m->vector.y * quality;
      total_quality+= quality;
    }
  }

  if (total_quality>0) {
    *found = true;
    result.x=result.x/(int)total_quality;
    result.y=result.y/(int)total_quality;
  }
  return result;         
}


/** Adds the windvector vector with quality quality to the list. */
void WindMeasurementList::addMeasurement(double Time,
                                         Vector vector, double alt, int quality){
  uint index;
  if (nummeasurementlist==MAX_MEASUREMENTS) {
    index = getLeastImportantItem(Time);
    delete measurementlist[index];
    nummeasurementlist--;
  } else {
    index = nummeasurementlist;
  }
  WindMeasurement * wind = new WindMeasurement;
  wind->vector.x = vector.x;
  wind->vector.y = vector.y;
  wind->quality=quality;
  wind->altitude=alt;
  wind->time= (long)Time;
  measurementlist[index] = wind;
  nummeasurementlist++;
}

/**
 * getLeastImportantItem is called to identify the item that should be
 * removed if the list is too full. Reimplemented from LimitedList.
 */
uint WindMeasurementList::getLeastImportantItem(double Time) {

  int maxscore=0;
  int score=0;
  unsigned int founditem= nummeasurementlist-1;

  for (int i=founditem;i>=0;i--) {

    //Calculate the score of this item. The item with the highest
    //score is the least important one.  We may need to adjust the
    //proportion of the quality and the elapsed time. Currently, one
    //quality-point (scale: 1 to 5) is equal to 10 minutes.

    score=600*(6-measurementlist[i]->quality);
    score+= (int)(Time - (double)measurementlist[i]->time);
    if (score>maxscore) {
      maxscore=score;
      founditem=i;
    }
  }
  return founditem;  
}


/*
int WindMeasurementList::compareItems(QCollection::Item s1, QCollection::Item s2) {
  //return the difference between the altitudes in item 1 and item 2
  return (int)(((WindMeasurement*)s1)->altitude - ((WindMeasurement*)s2)->altitude).getMeters();
}
*/

