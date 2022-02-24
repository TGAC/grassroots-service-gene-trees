/*
** Copyright 2014-2016 The Earlham Institute
** 
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
** 
**     http://www.apache.org/licenses/LICENSE-2.0
** 
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/**
 * @file
 * @brief
 */
/*
 * parental_genotype_service_data.h
 *
 *  Created on: 18 Nov 2018
 *      Author: tyrrells
 */

#ifndef GENE_TREES_SERVICE_DATA_H
#define GENE_TREES_SERVICE_DATA_H

#include "gene_trees_service_library.h"
#include "jansson.h"

#include "service.h"
#include "mongodb_tool.h"



/**
 * The configuration data used by the Gene Trees Service.
 *
 * @extends ServiceData
 */
typedef struct /*GENE_TREES_SERVICE_LOCAL*/ GeneTreesServiceData
{
	/** The base ServiceData. */
	ServiceData gtsd_base_data;


	/**
	 * @private
	 *
	 * The MongoTool to connect to the database where our data is stored.
	 */
	MongoTool *gtsd_mongo_p;


	/**
	 * @private
	 *
	 * The name of the database to use.
	 */
	const char *gtsd_database_s;


	/**
	 * @private
	 *
	 * The collection name to use.
	 */
	const char *gtsd_collection_s;


} GeneTreesServiceData;



#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef ALLOCATE_GENE_TREES_SERVICE_TAGS
	#define GENE_TREES_PREFIX GENE_TREES_SERVICE_LOCAL
	#define GENE_TREES_VAL(x)	= x
	#define GENE_TREES_CONCAT_VAL(x,y) = x y
#else
	#define GENE_TREES_PREFIX extern
	#define GENE_TREES_VAL(x)
	#define GENE_TREES_CONCAT_VAL(x,y) = x y
#endif

#endif 		/* #ifndef DOXYGEN_SHOULD_SKIP_THIS */

/** The prefix to use for Field Trial Service aliases. */
#define GT_GROUP_ALIAS_PREFIX_S "gene_trees"


#ifdef __cplusplus
extern "C"
{
#endif

GENE_TREES_SERVICE_LOCAL GeneTreesServiceData *AllocateGeneTreesServiceData (void);


GENE_TREES_SERVICE_LOCAL void FreeGeneTreesServiceData (GeneTreesServiceData *data_p);


GENE_TREES_SERVICE_LOCAL bool ConfigureGeneTreesService (GeneTreesServiceData *data_p, GrassrootsServer *grassroots_p);

#ifdef __cplusplus
}
#endif


#endif /* GENE_TREES_SERVICE_DATA_H */
