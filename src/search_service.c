/*
 ** Copyright 2014-2018 The Earlham Institute
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
/*
 * search_service.c
 *
 *  Created on: 24 Oct 2018
 *      Author: billy
 */

#include "search_service.h"
#include "gene_trees_service.h"


#include "audit.h"
#include "streams.h"
#include "math_utils.h"
#include "string_utils.h"

#include "string_parameter.h"
#include "unsigned_int_parameter.h"
#include "boolean_parameter.h"

/*
 * Static declarations
 */

static NamedParameterType S_GENE_ID = { "GT Gene", PT_STRING };
static NamedParameterType S_CLUSTER_ID = { "GT Cluster", PT_UNSIGNED_INT };
static NamedParameterType S_GENERATE_INDEXES = { "GT Generate Indexes", PT_BOOLEAN };


static const char *GetGeneTreesSearchServiceName (const Service *service_p);

static const char *GetGeneTreesSearchServiceDescription (const Service *service_p);

static const char *GetGeneTreesSearchServiceAlias (const Service *service_p);

static const char *GetGeneTreesSearchServiceInformationUri (const Service *service_p);

static ParameterSet *GetGeneTreesSearchServiceParameters (Service *service_p, DataResource *resource_p, User *user_p);

static bool GetGeneTreesSearchServiceParameterTypesForNamedParameters (const Service *service_p, const char *param_name_s, ParameterType *pt_p);

static void ReleaseGeneTreesSearchServiceParameters (Service *service_p, ParameterSet *params_p);

static ServiceJobSet *RunGeneTreesSearchService (Service *service_p, ParameterSet *param_set_p, User *user_p, ProvidersStateTable *providers_p);

static ParameterSet *IsResourceForGeneTreesSearchService (Service *service_p, DataResource *resource_p, Handler *handler_p);

static bool CloseGeneTreesSearchService (Service *service_p);

static ServiceMetadata *GetGeneTreesSearchServiceMetadata (Service *service_p);

static void DoSearch (ServiceJob *job_p, const char * const gene_s, const uint32 * const cluster_p, GeneTreesServiceData *data_p);


/*
 * API definitions
 */


Service *GetGeneTreesSearchService (GrassrootsServer *grassroots_p)
{
	Service *service_p = (Service *) AllocMemory (sizeof (Service));

	if (service_p)
		{
			GeneTreesServiceData *data_p = AllocateGeneTreesServiceData ();

			if (data_p)
				{
					if (InitialiseService (service_p,
																 GetGeneTreesSearchServiceName,
																 GetGeneTreesSearchServiceDescription,
																 GetGeneTreesSearchServiceAlias,
																 GetGeneTreesSearchServiceInformationUri,
																 RunGeneTreesSearchService,
																 IsResourceForGeneTreesSearchService,
																 GetGeneTreesSearchServiceParameters,
																 GetGeneTreesSearchServiceParameterTypesForNamedParameters,
																 ReleaseGeneTreesSearchServiceParameters,
																 CloseGeneTreesSearchService,
																 NULL,
																 false,
																 SY_SYNCHRONOUS,
																 (ServiceData *) data_p,
																 GetGeneTreesSearchServiceMetadata,
																 NULL,
																 grassroots_p))
						{
							if (ConfigureGeneTreesService (data_p, grassroots_p))
								{
									return service_p;
								}

						}		/* if (InitialiseService (.... */
					else
						{
							FreeGeneTreesServiceData (data_p);
						}
				}

			if (service_p)
				{
					FreeService (service_p);
				}

		}		/* if (service_p) */

	return NULL;
}



static const char *GetGeneTreesSearchServiceName (const Service * UNUSED_PARAM (service_p))
{
	return "GeneTrees search service";
}


static const char *GetGeneTreesSearchServiceDescription (const Service * UNUSED_PARAM (service_p))
{
	return "A service to get the parental data for given markers and populations";
}


static const char *GetGeneTreesSearchServiceAlias (const Service * UNUSED_PARAM (service_p))
{
	return GT_GROUP_ALIAS_PREFIX_S SERVICE_GROUP_ALIAS_SEPARATOR "search";
}

static const char *GetGeneTreesSearchServiceInformationUri (const Service * UNUSED_PARAM (service_p))
{
	return NULL;
}


static ParameterSet *GetGeneTreesSearchServiceParameters (Service *service_p, DataResource * UNUSED_PARAM (resource_p), User * UNUSED_PARAM (user_p))
{
	ParameterSet *param_set_p = AllocateParameterSet ("GeneTrees search service parameters", "The parameters used for the GeneTrees search service");

	if (param_set_p)
		{
			ServiceData *data_p = service_p -> se_data_p;
			Parameter *param_p = NULL;
			ParameterGroup *group_p = NULL;

			if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, S_GENE_ID.npt_type, S_GENE_ID.npt_name_s, "Gene", "The Gene ID to search for", NULL, PL_ALL)) != NULL)
				{
					if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (data_p, param_set_p, group_p, S_CLUSTER_ID.npt_name_s, "Cluster", "The Cluster ID to search for", NULL, PL_ALL)) != NULL)
						{
							if ((param_p = EasyCreateAndAddBooleanParameterToParameterSet(data_p, param_set_p, group_p, S_GENERATE_INDEXES.npt_name_s, "Indexes", "Ensure indexes for faster searching", NULL, PL_ADVANCED)) != NULL)
								{
									return param_set_p;
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", S_GENERATE_INDEXES.npt_name_s);
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", S_CLUSTER_ID.npt_name_s);
						}
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", S_GENE_ID.npt_name_s);
				}

			FreeParameterSet (param_set_p);
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate %s ParameterSet", GetGeneTreesSearchServiceName (service_p));
		}

	return NULL;
}


static bool GetGeneTreesSearchServiceParameterTypesForNamedParameters (const Service *service_p, const char *param_name_s, ParameterType *pt_p)
{
	const NamedParameterType params [] =
		{
			S_GENE_ID,
			S_CLUSTER_ID,
			S_GENERATE_INDEXES,
			NULL
		};

	return DefaultGetParameterTypeForNamedParameter (param_name_s, pt_p, params);
}



static void ReleaseGeneTreesSearchServiceParameters (Service * UNUSED_PARAM (service_p), ParameterSet *params_p)
{
	FreeParameterSet (params_p);
}


static bool CloseGeneTreesSearchService (Service *service_p)
{
	bool success_flag = true;

	FreeGeneTreesServiceData ((GeneTreesServiceData *) (service_p -> se_data_p));;

	return success_flag;
}


static ServiceJobSet *RunGeneTreesSearchService (Service *service_p, ParameterSet *param_set_p, User * UNUSED_PARAM (user_p), ProvidersStateTable * UNUSED_PARAM (providers_p))
{
	GeneTreesServiceData *data_p = (GeneTreesServiceData *) (service_p -> se_data_p);

	service_p -> se_jobs_p = AllocateSimpleServiceJobSet (service_p, NULL, "Gene Trees");

	if (service_p -> se_jobs_p)
		{
			ServiceJob *job_p = GetServiceJobFromServiceJobSet (service_p -> se_jobs_p, 0);

			LogParameterSet (param_set_p, job_p);

			SetServiceJobStatus (job_p, OS_FAILED_TO_START);

			if (param_set_p)
				{
					const char *gene_s = NULL;
					const uint32 *cluster_p = NULL;
					const bool *indexes_p = NULL;

					if (GetCurrentBooleanParameterValueFromParameterSet (param_set_p, S_GENERATE_INDEXES.npt_name_s, &indexes_p))
						{
							if (indexes_p && (*indexes_p))
								{
									if (!AddCollectionSingleIndex (data_p -> gtsd_mongo_p, data_p -> gtsd_database_s, data_p -> gtsd_collection_s, GTS_GENE_ID_S, NULL, true, false))
										{
											AddParameterErrorMessageToServiceJob (job_p, S_GENERATE_INDEXES.npt_name_s, S_GENERATE_INDEXES.npt_type, "Failed to add index for genes");
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add index for db \"%s\" collection \"%s\" field \"%s\"", data_p -> gtsd_database_s, data_p -> gtsd_collection_s, GTS_GENE_ID_S);
										}

									if (!AddCollectionSingleIndex (data_p -> gtsd_mongo_p, data_p -> gtsd_database_s, data_p -> gtsd_collection_s, GTS_CLUSTER_ID_S, NULL, false, false))
										{
											AddParameterErrorMessageToServiceJob (job_p, S_GENERATE_INDEXES.npt_name_s, S_GENERATE_INDEXES.npt_type, "Failed to add index for clusters");
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add index for db \"%s\" collection \"%s\" field \"%s\"", data_p -> gtsd_database_s, data_p -> gtsd_collection_s, GTS_GENE_ID_S);
										}
								}
						}


					if (GetCurrentStringParameterValueFromParameterSet (param_set_p, S_GENE_ID.npt_name_s, &gene_s))
						{
							if (IsStringEmpty (gene_s))
								{
									gene_s = NULL;
								}
						}		/* if (GetParameterValueFromParameterSet (param_set_p, S_MARKER.npt_name_s, &marker_value, true)) */

					GetCurrentUnsignedIntParameterValueFromParameterSet (param_set_p, S_CLUSTER_ID.npt_name_s, &cluster_p);

					if (gene_s || cluster_p)
						{
							DoSearch (job_p, gene_s, cluster_p, data_p);
						}


				}		/* if (param_set_p) */


			LogServiceJob (job_p);
		}		/* if (service_p -> se_jobs_p) */

	return service_p -> se_jobs_p;
}


static ServiceMetadata *GetGeneTreesSearchServiceMetadata (Service * UNUSED_PARAM (service_p))
{
	const char *term_url_s = CONTEXT_PREFIX_EDAM_ONTOLOGY_S "topic_0625";
	SchemaTerm *category_p = AllocateSchemaTerm (term_url_s, "Genotype and phenotype",
																							 "The study of genetic constitution of a living entity, such as an individual, and organism, a cell and so on, "
																							 "typically with respect to a particular observable phenotypic traits, or resources concerning such traits, which "
																							 "might be an aspect of biochemistry, physiology, morphology, anatomy, development and so on.");

	if (category_p)
		{
			SchemaTerm *subcategory_p;

			term_url_s = CONTEXT_PREFIX_EDAM_ONTOLOGY_S "operation_0304";
			subcategory_p = AllocateSchemaTerm (term_url_s, "Query and retrieval", "Search or query a data resource and retrieve entries and / or annotation.");

			if (subcategory_p)
				{
					ServiceMetadata *metadata_p = AllocateServiceMetadata (category_p, subcategory_p);

					if (metadata_p)
						{
							SchemaTerm *input_p;

							term_url_s = CONTEXT_PREFIX_EDAM_ONTOLOGY_S "data_0968";
							input_p = AllocateSchemaTerm (term_url_s, "Keyword",
																						"Boolean operators (AND, OR and NOT) and wildcard characters may be allowed. Keyword(s) or phrase(s) used (typically) for text-searching purposes.");

							if (input_p)
								{
									if (AddSchemaTermToServiceMetadataInput (metadata_p, input_p))
										{
											SchemaTerm *output_p;


											/* Genotype */
											term_url_s = CONTEXT_PREFIX_EXPERIMENTAL_FACTOR_ONTOLOGY_S "EFO_0000513";
											output_p = AllocateSchemaTerm (term_url_s, "genotype", "Information, making the distinction between the actual physical material "
																										 "(e.g. a cell) and the information about the genetic content (genotype).");

											if (output_p)
												{
													if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p))
														{
															return metadata_p;
														}		/* if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p)) */
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add output term %s to service metadata", term_url_s);
															FreeSchemaTerm (output_p);
														}

												}		/* if (output_p) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate output term %s for service metadata", term_url_s);
												}

										}		/* if (AddSchemaTermToServiceMetadataInput (metadata_p, input_p)) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add input term %s to service metadata", term_url_s);
											FreeSchemaTerm (input_p);
										}

								}		/* if (input_p) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate input term %s for service metadata", term_url_s);
								}

							FreeServiceMetadata (metadata_p);
						}		/* if (metadata_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate service metadata");
						}

				}		/* if (subcategory_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate sub-category term %s for service metadata", term_url_s);
				}

		}		/* if (category_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate category term %s for service metadata", term_url_s);
		}

	return NULL;
}


static ParameterSet *IsResourceForGeneTreesSearchService (Service * UNUSED_PARAM (service_p), DataResource * UNUSED_PARAM (resource_p), Handler * UNUSED_PARAM (handler_p))
{
	return NULL;
}




static void DoSearch (ServiceJob *job_p, const char * const gene_s, const uint32 * const cluster_p, GeneTreesServiceData *data_p)
{
	OperationStatus status = OS_FAILED_TO_START;
	bson_t *query_p = bson_new ();

	if (query_p)
		{
			bool success_flag = true;
			
			if (gene_s)
				{
					if (!BSON_APPEND_UTF8 (query_p, GTS_GENE_ID_S, gene_s))
						{
							success_flag = false;
							PrintBSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, query_p, "Failed to add \"%s\": \"%s\"", GTS_GENE_ID_S, gene_s);
						}
				}

			if (cluster_p)
				{
					if (!BSON_APPEND_INT32 (query_p, GTS_CLUSTER_ID_S, *cluster_p))
						{
							success_flag = false;							
							PrintBSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, query_p, "Failed to add \"%s\": " UINT32_FMT, GTS_CLUSTER_ID_S, *cluster_p);
						}
				}

			if (success_flag)
				{
					json_t *results_p = GetAllMongoResultsAsJSON (data_p -> gtsd_mongo_p, query_p, NULL);

					if (results_p)
						{
							const size_t num_results = json_array_size (results_p);
							size_t i = 0;
							size_t num_added = 0;
							char *query_s = NULL;
							
							if (cluster_p)
								{
									char *cluster_s = ConvertUnsignedIntegerToString (*cluster_p);
									
									if (cluster_s)
										{
											if (gene_s)
												{
													query_s = ConcatenateVarargsStrings (gene_s, " - ", cluster_s, NULL);
													FreeCopiedString (cluster_s);
												} 
											else
												{
													query_s = cluster_s;
												}
										}
									else if (gene_s)
										{
											query_s = (char *) gene_s;
										}
								}

							while (i < num_results)
								{
									json_t *entry_p = json_array_get (results_p, i);
									json_t *resource_p = NULL;
									char *index_s = ConvertSizeTToString (i);
									char *title_s = NULL;

									if (query_s)
										{
											if (index_s)
												{
													title_s = ConcatenateVarargsStrings (query_s, " - ", index_s, NULL);
													FreeCopiedString (index_s);
												}
											else
												{
													PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to convert " SIZET_FMT " to string", i);
												}

										}

									resource_p = GetDataResourceAsJSONByParts (PROTOCOL_INLINE_S, NULL, title_s ? title_s : query_s, entry_p);

									if (title_s)
										{
											FreeCopiedString (title_s);
										}

									if (resource_p)
										{
											if (AddResultToServiceJob (job_p, resource_p))
												{
													++ num_added;
												}
											else
												{
													AddGeneralErrorMessageToServiceJob (job_p, "Failed to add one or more hits to result");
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, resource_p, "Failed to add result " SIZET_FMT " for query \"%s\", %d to service job", i, gene_s ? gene_s : "NULL", cluster_p ? *cluster_p : -1);
													json_decref (resource_p);
												}
										}
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create resource for result " SIZET_FMT " to query \"%s\": \%d", i, gene_s ? gene_s : "NULL", cluster_p ? *cluster_p : -1);
										}

									++ i;
								}		/* while ((i < num_results) && success_flag) */

							if (num_added == num_results)
								{
									status = OS_SUCCEEDED;
								}
							else if (num_added > 0)
								{
									status = OS_PARTIALLY_SUCCEEDED;
								}
							else
								{
									status = OS_FAILED;
								}

							if (query_s && (query_s != gene_s))
								{
									FreeCopiedString (query_s);
								}

							json_decref (results_p);
						}		/* if (results_p) */

				}		/* if (BSON_APPEND_UTF8 (query_p, PGS_POPULATION_NAME_S, gene_s)) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to append \"%s\", %d to query", gene_s ? gene_s : "NULL", cluster_p ? *cluster_p : -1);
				}
			bson_destroy (query_p);
		}		/* if (query_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create query for \"%s\", %d", gene_s ? gene_s : "NULL", cluster_p ? *cluster_p : -1);
		}

	SetServiceJobStatus (job_p, status);
}



