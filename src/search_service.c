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
#include "boolean_parameter.h"

/*
 * Static declarations
 */

static NamedParameterType S_GENE_ID = { "Gene", PT_KEYWORD };


static const char *GetGeneTreesSearchServiceName (const Service *service_p);

static const char *GetGeneTreesSearchServiceDescription (const Service *service_p);

static const char *GetGeneTreesSearchServiceAlias (const Service *service_p);

static const char *GetGeneTreesSearchServiceInformationUri (const Service *service_p);

static ParameterSet *GetGeneTreesSearchServiceParameters (Service *service_p, Resource *resource_p, UserDetails *user_p);

static bool GetGeneTreesSearchServiceParameterTypesForNamedParameters (const Service *service_p, const char *param_name_s, ParameterType *pt_p);

static void ReleaseGeneTreesSearchServiceParameters (Service *service_p, ParameterSet *params_p);

static ServiceJobSet *RunGeneTreesSearchService (Service *service_p, ParameterSet *param_set_p, UserDetails *user_p, ProvidersStateTable *providers_p);

static ParameterSet *IsResourceForGeneTreesSearchService (Service *service_p, Resource *resource_p, Handler *handler_p);

static bool CloseGeneTreesSearchService (Service *service_p);

static ServiceMetadata *GetGeneTreesSearchServiceMetadata (Service *service_p);

static void DoSearch (ServiceJob *job_p, const char * const gene_s, GeneTreesServiceData *data_p);

static bool CopyJSONString (const json_t *src_p, const char *src_key_s, json_t *dest_p, const char *dest_key_s);

static bool CopyJSONObject (const json_t *src_p, const char *src_key_s, json_t *dest_p, const char *dest_key_s);


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


static ParameterSet *GetGeneTreesSearchServiceParameters (Service *service_p, Resource * UNUSED_PARAM (resource_p), UserDetails * UNUSED_PARAM (user_p))
{
	ParameterSet *param_set_p = AllocateParameterSet ("GeneTrees search service parameters", "The parameters used for the GeneTrees search service");

	if (param_set_p)
		{
			ServiceData *data_p = service_p -> se_data_p;
			Parameter *param_p = NULL;
			ParameterGroup *group_p = NULL;

			if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, S_GENE_ID.npt_type, S_GENE_ID.npt_name_s, "Gene", "The Gene ID to search for", NULL, PL_ALL)) != NULL)
				{
					return param_set_p;
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
	bool success_flag = true;

	if (strcmp (param_name_s, S_GENE_ID.npt_name_s) == 0)
		{
			*pt_p = S_GENE_ID.npt_type;
		}
	else
		{
			success_flag = false;
		}

	return success_flag;
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


static ServiceJobSet *RunGeneTreesSearchService (Service *service_p, ParameterSet *param_set_p, UserDetails * UNUSED_PARAM (user_p), ProvidersStateTable * UNUSED_PARAM (providers_p))
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

					if (GetCurrentStringParameterValueFromParameterSet (param_set_p, S_GENE_ID.npt_name_s, &gene_s))
						{
							if (!IsStringEmpty (gene_s))
								{
									DoSearch (job_p, gene_s, data_p);
								}

						}		/* if (GetParameterValueFromParameterSet (param_set_p, S_MARKER.npt_name_s, &marker_value, true)) */

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


static ParameterSet *IsResourceForGeneTreesSearchService (Service * UNUSED_PARAM (service_p), Resource * UNUSED_PARAM (resource_p), Handler * UNUSED_PARAM (handler_p))
{
	return NULL;
}




static void DoSearch (ServiceJob *job_p, const char * const gene_s, GeneTreesServiceData *data_p)
{
	OperationStatus status = OS_FAILED_TO_START;
	bson_t *query_p = bson_new ();

	if (query_p)
		{
			if (BSON_APPEND_UTF8 (query_p, GTS_GENE_ID_S, gene_s))
				{
					json_t *results_p = GetAllMongoResultsAsJSON (data_p -> gtsd_mongo_p, query_p, NULL);

					if (results_p)
						{
							const size_t num_results = json_array_size (results_p);
							size_t i = 0;
							size_t num_added = 0;
							bool success_flag = true;

							while ((i < num_results) && success_flag)
								{
									const json_t *entry_p = json_array_get (results_p, i);
									json_t *resource_p = GetResourceAsJSONByParts (PROTOCOL_INLINE_S, NULL, gene_s, entry_p);

									if (resource_p)
										{
											if (AddResultToServiceJob (job_p, resource_p))
												{
													++ num_added;
												}
											else
												{
													json_decref (resource_p);
												}
										}

									++ i;
								}		/* while ((i < num_results) && success_flag) */

							if (!success_flag)
								{
									json_decref (results_p);
									results_p = NULL;
								}

							json_decref (results_p);
						}		/* if (results_p) */

				}		/* if (BSON_APPEND_UTF8 (query_p, PGS_POPULATION_NAME_S, gene_s)) */


			bson_destroy (query_p);
		}		/* if (query_p) */

	SetServiceJobStatus (job_p, status);
}




static bool CopyJSONString (const json_t *src_p, const char *src_key_s, json_t *dest_p, const char *dest_key_s)
{
	bool success_flag = false;
	const char *value_s = GetJSONString (src_p, src_key_s);

	if (value_s)
		{
			if (SetJSONString (dest_p, dest_key_s ? dest_key_s : src_key_s, value_s))
				{
					success_flag = true;
				}
		}

	return success_flag;
}


static bool CopyJSONObject (const json_t *src_p, const char *src_key_s, json_t *dest_p, const char *dest_key_s)
{
	bool success_flag = false;
	json_t *value_p = json_object_get (src_p, src_key_s);

	if (value_p)
		{
			if (json_object_set (dest_p, dest_key_s ? dest_key_s : src_key_s, value_p) == 0)
				{
					success_flag = true;
				}
		}

	return success_flag;
}



