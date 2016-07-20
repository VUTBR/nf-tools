#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include "ffilter_internal.h"
#include "ffilter_gram.h"
#include "ffilter.h"

/*
 * \brief Convert unit character to positive power of 10
 * \param[in] unit Suffix of number
 * \return Scale eg. (1k -> 1000) etc.
 */
uint64_t get_unit(char *unit)
{
	if (strlen(unit) > 1) return 0;

	switch (*unit) {
	case 'k':
	case 'K':
		return FF_SCALING_FACTOR;
	case 'm':
	case 'M':
		return FF_SCALING_FACTOR * FF_SCALING_FACTOR;
	case 'g':
	case 'G':
		return FF_SCALING_FACTOR * FF_SCALING_FACTOR * FF_SCALING_FACTOR;
	case 'T':
		return FF_SCALING_FACTOR * FF_SCALING_FACTOR * FF_SCALING_FACTOR * FF_SCALING_FACTOR;
	case 'E':
		return FF_SCALING_FACTOR * FF_SCALING_FACTOR * FF_SCALING_FACTOR * FF_SCALING_FACTOR * FF_SCALING_FACTOR;
	default:
		return 0;
	}

}

/*
 * \brief Function adds support for k/M/G/T/E suffixes to strtoll
 * \param[in] num Literal number
 * \param endptr Place to store an address where conversion finised
 */
int64_t strtoll_unit(char *num, char**endptr)
{
	int64_t tmp64;
	int64_t mult = 0;

	tmp64 = strtoll(num, endptr, 10);
	if (!**endptr) {
		return tmp64;
	} else if (!*(*endptr+1)) {
		mult = get_unit(*endptr);
		if (mult != 0) {
			/*Move conversion end potinter by one*/
			*endptr = (*endptr + 1);
		}
	}
	return tmp64*mult;
}

/* convert string into uint64_t */
/* also converts string with units (64k -> 64000) */
int str_to_uint(char *str, int type, char **res, int *vsize)
{

	uint64_t tmp64;
	uint32_t tmp32;
	uint32_t tmp16;
	uint32_t tmp8;
	void *tmp, *ptr;

	char* endptr;
	tmp64 = strtoll_unit(str, &endptr);
	if (*endptr){
		return 0;
	}

	switch (type) {
	case FF_TYPE_UINT64:
		*vsize = sizeof(uint64_t);
		tmp = &tmp64;
		break;
	case FF_TYPE_UINT32:
		*vsize = sizeof(uint32_t);
		tmp32 = tmp64;
		tmp = &tmp32;
		break;
	case FF_TYPE_UINT16:
		*vsize = sizeof(uint16_t);
		tmp16 = tmp64;
		tmp = &tmp16;
		break;
	case FF_TYPE_UINT8:
		*vsize = sizeof(uint8_t);
		tmp8 = tmp64;
		tmp = &tmp8;
		break;
	default: return 0;
	}

	ptr = malloc(*vsize);

	if (ptr == NULL) {
		return 0;
	}

	memcpy(ptr, tmp, *vsize);

	*res = ptr;

	return 1;
}

/* convert string into int64_t */
/* also converts string with units (64k -> 64000) */
int str_to_int(char *str, int type, char **res, int *vsize)
{

	int64_t tmp64;
	int32_t tmp32;
	int32_t tmp16;
	int32_t tmp8;
	void *tmp, *ptr;

	char *endptr;
	tmp64 = strtoll_unit(str, &endptr);
	if (*endptr){
		return 0;
	}

	switch (type) {
	case FF_TYPE_INT64:
		*vsize = sizeof(int64_t);
		tmp = &tmp64;
		break;
	case FF_TYPE_INT32:
		*vsize = sizeof(int32_t);
		tmp32 = tmp64;
		tmp = &tmp32;
		break;
	case FF_TYPE_INT16:
		*vsize = sizeof(int16_t);
		tmp16 = tmp64;
		tmp = &tmp16;
		break;
	case FF_TYPE_INT8:
		*vsize = sizeof(int8_t);
		tmp8 = tmp64;
		tmp = &tmp8;
		break;
	default: return 0;
	}

	ptr = malloc(*vsize);

	if (ptr == NULL) {
		return 0;
	}

	memcpy(ptr, tmp, *vsize);

	*res = ptr;

	return 1;
}

/* convert string into lnf_ip_t */
//TODO: Solve masked addresses (maybe add one more ip addr as mask so eval wold be & together ip and mask then eval
int str_to_addr(ff_t *filter, char *str, char **res, int *numbits)
{
	ff_net_t *ptr;

	ptr = malloc(sizeof(ff_net_t));

	if (ptr == NULL) {
		return 0;
	}

	char *saveptr;

	char *ip_str = strdup(str);
	char *ip;
	char *mask;
	char suffix[8] = "";

	int ip_ver = 4;

	memset(ptr, 0x0, sizeof(ff_net_t));

	*numbits = 0;

	*res = (char *)ptr;

	ip = strtok_r(ip_str, "\\/", &saveptr);
	mask = strtok_r(NULL, "\\/", &saveptr);

	/* Try to guess ip Version */
	if (strchr(ip_str, ':')) {
		ip_ver = 6;
	}

	if (mask != NULL) {
		*numbits = strtoul(mask, &saveptr, 10);
		if (*saveptr) {
			return 1;
		}

		int req_oct;
		int octet;
		char *ip_dup = strdup(ip_str);
		/* Using shifts to right instead of division */
		switch (ip_ver) {
		case 4:
			if (*numbits > 32) { *numbits = 32; }
			/* If string passes this filter, it has enough octets, rest ist concatenated */
			req_oct = (*numbits >> 3) + ((*numbits & 0b111) > 0); //Get number of reqired octets
			octet = 0;
			/* Check for required octets, note that inet_pton does the job */
			for (ip = strtok_r(ip_dup, ".", &saveptr); ip != NULL; octet++) {
				ip = strtok_r(NULL, ".", &saveptr);
			}
			if (octet < req_oct) {
				free(ip_dup);
				free(ip_str);
				return 0;
			}

			/* Concat missing octets */
			for (suffix[0] = 0; octet < 4; octet++) {
				strcat(suffix, ".0");
			}
			ip = realloc(ip_str, strlen(ip_str) + 8 * sizeof(char));
			if (ip) {
				strcat(ip, suffix);
			} else { return 0; }

			uint32_t mask = ~0U;
			ptr->mask.data[3] = htonl(~(mask >> (*numbits)));
			break;
		case 6:
			if (*numbits > 128) { *numbits = 128; }
			/* For now hope for the best and except ipv6 in xxxx:etc::0 padded form */
			req_oct = (*numbits >> 5) + ((*numbits & 0b11111) > 0); //Get number of reqired octets
			octet = 0;
			/* Check for required octets, note that inet_pton does the job */

			for (ip = strtok_r(ip_dup, ":", &saveptr); ip != NULL; octet++) {
				ip = strtok_r(NULL, ":", &saveptr);
			}
			if (octet < req_oct && strstr(ip_str,"::") == NULL) {
				free(ip_dup);
				free(ip_str);
				return 0;
			}

			int x;
			for (x = 0; x < (*numbits >> 5); x++) {
				ptr->mask.data[x] = ~0U;
			}
			if (x < 4) {
				uint32_t mask = ~0U;
				ptr->mask.data[x] = htonl(~(mask >> (*numbits & 0b11111)));
			}
			break;
		}
		free(ip_dup);
	} else {
		/* Mask was not given -> compare whole ip */
		memset(&ptr->mask, ~0, sizeof(ff_ip_t));
	}

	if (inet_pton(AF_INET, ip_str, &(ptr->ip.data[3]))) {
		ptr->mask.data[0] = ptr->mask.data[1] = ptr->mask.data[2] = 0;
		ptr->ip.data[3] &= ptr->mask.data[3];
		free(ip_str);
		return 1;
	}

	if (inet_pton(AF_INET6, ip_str, &ptr->ip)) {
		for (int x = 0; x < 4; x++) {
			ptr->ip.data[x] &= ptr->mask.data[x];
		}
		free(ip_str);
		return 1;
	}

	ff_set_error(filter, "Can't convert '%s' into IP address", str);

	free(ip_str);
	return 0;
}

//TODO: Solve this
//ff_error_t str_to_mac()

ff_error_t ff_type_cast(yyscan_t *scanner, ff_t *filter, char *valstr, ff_node_t* node) {

	/* determine field type and assign data to lvalue */
	switch (node->type) {
	case FF_TYPE_UINT64:
	case FF_TYPE_UINT32:
	case FF_TYPE_UINT16:
	case FF_TYPE_UINT8:
		if (str_to_uint(valstr, node->type, &node->value, &node->vsize) == 0) {
			ff_set_error(filter, "Can't convert '%s' into numeric value", valstr);
			return FF_ERR_OTHER;
		}
		break;
	case FF_TYPE_INT64:
	case FF_TYPE_INT32:
	case FF_TYPE_INT16:
	case FF_TYPE_INT8:
		if (str_to_int(valstr, node->type, &node->value, &node->vsize) == 0) {
			ff_set_error(filter, "Can't convert '%s' into numeric value", valstr);

			return FF_ERR_OTHER;
		}
		break;
	case FF_TYPE_ADDR:
		if (str_to_addr(filter, valstr, &node->value, &node->numbits) == 0) {
			return FF_ERR_OTHER;
		}
		node->vsize = sizeof(ff_net_t);
		break;

		/* unsigned with undefined data size (internally mapped to uint64_t in network order) */
	case FF_TYPE_UNSIGNED_BIG:
	case FF_TYPE_UNSIGNED:
		if (str_to_uint(valstr, FF_TYPE_UINT64, &node->value, &node->vsize) == 0) {
			node->value = calloc(1, sizeof(uint64_t));
			node->vsize = sizeof(uint64_t);
			if (node->value == NULL || filter->options.ff_rval_map_func == NULL) {
				node->vsize = 0;
				ff_set_error(filter, "Can't convert '%s' into numeric value", valstr);
				return FF_ERR_OTHER;
			} else if (filter->options.ff_rval_map_func(filter, valstr, node->field,
								    (uint64_t *) node->value) != FF_OK) {
				free(node->value);
				node->vsize = 0;
				ff_set_error(filter, "Can't convert '%s' into numeric value", valstr);
				return FF_ERR_OTHER;
			}
		}
		break;
	case FF_TYPE_SIGNED_BIG:
	case FF_TYPE_SIGNED:
		if (str_to_uint(valstr, FF_TYPE_INT64, &node->value, &node->vsize) == 0) {
			ff_set_error(filter, "Can't convert '%s' into numeric value", valstr);
			return FF_ERR_OTHER;
		}
		break;
	case FF_TYPE_STRING:
		if ((node->value = strdup(valstr)) != NULL) {
			node->vsize = strlen(valstr);
			break;
		}
		return FF_ERR_NOMEM;
	case FF_TYPE_MAC:
		/*if (str_to_mac(valstr, &node->value) == 0){
			return FF_ERR_OTHER;
		}
	case FF_TYPE_TIMESTAMP:
		*/
	default:
		return FF_ERR_OTHER;
	}
	return FF_OK;
}


/* set error to error buffer */
/* set error string */
void ff_set_error(ff_t *filter, char *format, ...) {
va_list args;

	va_start(args, format);
	vsnprintf(filter->error_str, FF_MAX_STRING - 1, format, args);
	va_end(args);
}

/* get error string */
const char* ff_error(ff_t *filter, const char *buf, int buflen) {

	strncpy((char *)buf, filter->error_str, buflen - 1);
	return buf;

}

//TODO: suppport more ids
ff_node_t* ff_branch_node(ff_node_t *node, ff_oper_t oper, ff_lvalue_t* lvalue) {

	ff_node_t *nodeR = ff_new_node(NULL, NULL, NULL, oper, NULL);
	if (nodeR == NULL){
		return NULL;
	}
	ff_node_t *root = ff_new_node(NULL, NULL, node, oper, nodeR);
	if (root == NULL) {
		ff_free_node(nodeR);
		return NULL;
	}

	*nodeR = *node;
	nodeR->field = lvalue->id2;

	nodeR->value = malloc(nodeR->vsize);
	memcpy(nodeR->value, node->value, nodeR->vsize);

	if (nodeR->value == NULL) {
		ff_free_node(nodeR);
		ff_free_node(root);
		return NULL;
	}
	return root;
}

/* Add leaf entry into expr tree */
ff_node_t* ff_new_leaf(yyscan_t scanner, ff_t *filter, char *fieldstr, ff_oper_t oper, char *valstr) {
	//int field;
	ff_node_t *node;
	ff_node_t *retval;
	ff_lvalue_t lvalue;

	int multinode = 1;
	ff_oper_t root_oper = -1;

	retval = NULL;

	/* callback to fetch field type and additional info */
	if (filter->options.ff_lookup_func == NULL) {
		ff_set_error(filter, "Filter lookup function not defined");
		return NULL;
	}

	memset(&lvalue, 0x0, sizeof(ff_lvalue_t));
	lvalue.num = 0;
	lvalue.more = NULL;

	switch (*fieldstr) {
	case '|':
		root_oper = FF_OP_OR;
		fieldstr++;
		break;
	case '&':
		root_oper = FF_OP_AND;
		fieldstr++;
		break;
	default:
		multinode = 0;
	}

	do { /* Mem guard block, break on error, let me know if this is silly solution */
		if (filter->options.ff_lookup_func(filter, fieldstr, &lvalue) != FF_OK) {

			ff_set_error(filter, "Can't lookup field type for %s", fieldstr);
			retval = NULL;
			break;
		}

			/* Change EQ evaluation for flags */
		if (lvalue.options & FFOPTS_FLAGS && oper == FF_OP_EQ) {
			oper = FF_OP_ISSET;
		}

		node = ff_new_node(scanner, filter, NULL, oper, NULL);
		if (node == NULL) {
			retval = NULL;
			break;
		}

		node->type = lvalue.type;
		node->field = lvalue.id;

		if (oper == FF_OP_IN) {
			//Htab nieje moc dobre riesenie
			//node->value = ff_htab_from_node(node, (ff_node_t*)valstr);
			node->value = valstr;
			retval = node;
			break;
		}

		if(ff_type_cast(scanner, filter, valstr, node) != FF_OK) {
			retval = NULL;
			ff_free_node(node);
			break;
		}

		node->left = NULL;
		node->right = NULL;

		if (lvalue.id2.index == 0) {
			free(lvalue.more);
			if (multinode) {
				ff_free_node(node);
				retval = NULL;
			}
			retval = node;
		} else {
			//Setup nodes in or configuration for pair fields (src/dst etc.)
			ff_node_t* new_root;
			new_root = ff_branch_node(node,
						  root_oper == -1 ? FF_OP_OR : root_oper,
						  &lvalue);
			if (new_root == NULL) {
				ff_free_node(node);
				break;
			}
			retval = new_root;
		}
	} while (0);

	free(lvalue.more);
	return retval;
}

/* add node entry into expr tree */
ff_node_t* ff_new_node(yyscan_t scanner, ff_t *filter, ff_node_t* left, ff_oper_t oper, ff_node_t* right) {

	ff_node_t *node;

	node = malloc(sizeof(ff_node_t));

	if (node == NULL) {
		return NULL;
	}

	node->vsize = 0;
	node->type = 0;
	node->oper = oper;

	node->left = left;
	node->right = right;

	return node;
}

/* add new item to tree */
ff_node_t* ff_new_mval(yyscan_t scanner, ff_t *filter, char *valstr, ff_oper_t oper, ff_node_t* nptr) {

	ff_node_t *node;

	node = malloc(sizeof(ff_node_t));

	if (node == NULL) {
		return NULL;
	}

	node->vsize = strlen(valstr);
	node->value = strdup(valstr);
	node->type = FF_TYPE_STRING;
	node->oper = oper;

	node->left = NULL;
	node->right = nptr;

	return node;
}

/* evaluate node in tree or proces subtree */
/* return 0 - false; 1 - true; -1 - error  */
int ff_eval_node(ff_t *filter, ff_node_t *node, void *rec) {
	char buf[FF_MAX_STRING];
	int left, right, res;
	size_t size;

	if (node == NULL) {
		return -1;
	}

	left = 0;

	/* go deeper into tree */
	if (node->left != NULL ) {
		left = ff_eval_node(filter, node->left, rec);

		/* do not evaluate if the result is obvious */
		if (node->oper == FF_OP_NOT)			{ return !left; };
		if (node->oper == FF_OP_OR  && left == 1)	{ return 1; };
		if (node->oper == FF_OP_AND && left == 0)	{ return 0; };
	}

	if (node->right != NULL ) {
		right = ff_eval_node(filter, node->right, rec);

		switch (node->oper) {
		case FF_OP_NOT: return !right;
		case FF_OP_OR:  return left || right;
		case FF_OP_AND: return left && right;
		default: break;
		}
	}

	/* operations on leaf -> compare values  */
	/* going to be callback */
	if (filter->options.ff_data_func(filter, rec, node->field, buf, &size) != FF_OK) {
		ff_set_error(filter, "Can't get data");
		return -1;
	}


	switch (node->oper) {

	case FF_OP_EQ:

		switch (node->type) {
		case FF_TYPE_UINT64: return *(uint64_t *) & buf == *(uint64_t *) node->value;
		case FF_TYPE_UINT32: return *(uint32_t *) & buf == *(uint32_t *) node->value;
		case FF_TYPE_UINT16: return *(uint16_t *) & buf == *(uint16_t *) node->value;
		case FF_TYPE_UINT8: return *(uint8_t *) & buf == *(uint8_t *) node->value;
		case FF_TYPE_INT64: return *(int64_t *) & buf == *(int64_t *) node->value;
		case FF_TYPE_INT32: return *(int32_t *) & buf == *(int32_t *) node->value;
		case FF_TYPE_INT16: return *(int16_t *) & buf == *(int16_t *) node->value;
		case FF_TYPE_INT8: return *(int8_t *) & buf == *(int8_t *) node->value;

		case FF_TYPE_DOUBLE: return *(double *) &buf == *(double *) node->value;
		case FF_TYPE_STRING: return !strcmp((char *) &buf, node->value);

		case FF_TYPE_UNSIGNED_BIG:
			if (size > node->vsize) { return -1; }                /* too big integer */
			switch (size) {
			case sizeof(uint8_t): return *(uint8_t *) buf == *(uint64_t * )(node->value);
			case sizeof(uint16_t): return ntohs(*(uint16_t *) buf) == *(uint64_t * )(node->value);
			case sizeof(uint32_t): return ntohl(*(uint32_t *) buf) == *(uint64_t * )(node->value);
			case sizeof(uint64_t): return ntohll(*(uint64_t *) buf) == *(uint64_t * )(node->value);
			default: return -1;
			}
		case FF_TYPE_UNSIGNED:
			if (size > node->vsize) { return -1; }                /* too big integer */
			switch (size) {
			case sizeof(uint8_t): return *(uint8_t *) buf == *(uint64_t * )(node->value);
			case sizeof(uint16_t): return *(uint16_t *) buf == *(uint64_t * )(node->value);
			case sizeof(uint32_t): return *(uint32_t *) buf == *(uint64_t * )(node->value);
			case sizeof(uint64_t): return *(uint64_t *) buf == *(uint64_t * )(node->value);
			default: return -1;
			}
		case FF_TYPE_SIGNED_BIG:
			if (size > node->vsize) { return -1; }                /* too big integer */
			switch (size) {
			case sizeof(int8_t): return *(int8_t *) buf == *(int64_t * )(node->value);
			case sizeof(int16_t): return ntohs(*(int16_t *) buf) == *(int64_t * )(node->value);
			case sizeof(int32_t): return ntohl(*(int32_t *) buf) == *(int64_t * )(node->value);
			case sizeof(int64_t): return ntohll(*(int64_t *) buf) == *(int64_t * )(node->value);
			default: return -1;
			}
		case FF_TYPE_SIGNED:
			if (size > node->vsize) { return -1; }                /* too big integer */
			switch (size) {
			case sizeof(int8_t): return *(int8_t *) buf == *(int64_t * )(node->value);
			case sizeof(int16_t): return *(int16_t *) buf == *(int64_t * )(node->value);
			case sizeof(int32_t): return *(int32_t *) buf == *(int64_t * )(node->value);
			case sizeof(int64_t): return *(int64_t *) buf == *(int64_t * )(node->value);
			default: return -1;
			}
		case FF_TYPE_ADDR:
				/* Compare masked ip addresses*/
			switch (size) {
			case sizeof(ff_ip_t):
				//Try comparation in xmm / or check if vectorised after optimizations
				res = 1;
				for (int x = 0; x < 4; x++) {
					/* Does not work for comparation GT LT some adaptation might work*/
					res = res && (((*(ff_ip_t *) buf).data[x] &
					    ((ff_net_t *) node->value)->mask.data[x]) ==
					    ((ff_net_t *) node->value)->ip.data[x]);
				}
				return res;
			case sizeof(uint32_t):
				if (!((ff_net_t *) node->value)->mask.data[3]) {
					return -1;
				}
				return (((*(uint32_t *) buf) &
				    ((ff_net_t *) node->value)->mask.data[3]) ==
				    ((ff_net_t *) node->value)->ip.data[3]);
				/* Eval fails if no bits are compared */
			default: return -1;
			}
		default: return -1;
		}

	case FF_OP_GT:

		switch (node->type) {
		case FF_TYPE_UINT64: return *(uint64_t *) & buf > *(uint64_t *) node->value;
		case FF_TYPE_UINT32: return *(uint32_t *) & buf > *(uint32_t *) node->value;
		case FF_TYPE_UINT16: return *(uint16_t *) & buf > *(uint16_t *) node->value;
		case FF_TYPE_UINT8: return *(uint8_t *) & buf > *(uint8_t *) node->value;
		case FF_TYPE_INT64: return *(int64_t *) & buf > *(int64_t *) node->value;
		case FF_TYPE_INT32: return *(int32_t *) & buf > *(int32_t *) node->value;
		case FF_TYPE_INT16: return *(int16_t *) & buf > *(int16_t *) node->value;
		case FF_TYPE_INT8: return *(int8_t *) & buf > *(int8_t *) node->value;

		case FF_TYPE_DOUBLE: return *(double *) &buf > *(double *) node->value;
		case FF_TYPE_STRING: return strcmp((char *) &buf, node->value) > 0;

		case FF_TYPE_UNSIGNED_BIG:
			if (size > node->vsize) { return -1; }                /* too big integer */
			switch (size) {
			case sizeof(uint8_t): return *(uint8_t *) buf > *(uint64_t * )(node->value);
			case sizeof(uint16_t): return ntohs(*(uint16_t *) buf) > *(uint64_t * )(node->value);
			case sizeof(uint32_t): return ntohl(*(uint32_t *) buf) > *(uint64_t * )(node->value);
			case sizeof(uint64_t): return ntohll(*(uint64_t *) buf) > *(uint64_t * )(node->value);
			default: return -1;
			}
		case FF_TYPE_UNSIGNED:
			if (size > node->vsize) { return -1; }                /* too big integer */
			switch (size) {
			case sizeof(uint8_t): return *(uint8_t *) buf > *(uint64_t * )(node->value);
			case sizeof(uint16_t): return *(uint16_t *) buf > *(uint64_t * )(node->value);
			case sizeof(uint32_t): return *(uint32_t *) buf > *(uint64_t * )(node->value);
			case sizeof(uint64_t): return *(uint64_t *) buf > *(uint64_t * )(node->value);
			default: return -1;
			}
		case FF_TYPE_SIGNED_BIG:
			if (size > node->vsize) { return -1; }                /* too big integer */
			switch (size) {
			case sizeof(int8_t): return *(int8_t *) buf > *(int64_t * )(node->value);
			case sizeof(int16_t): return ntohs(*(int16_t *) buf) > *(int64_t * )(node->value);
			case sizeof(int32_t): return ntohl(*(int32_t *) buf) > *(int64_t * )(node->value);
			case sizeof(int64_t): return ntohll(*(int64_t *) buf) > *(int64_t * )(node->value);
			default: return -1;
			}
		case FF_TYPE_SIGNED:
			if (size > node->vsize) { return -1; }                /* too big integer */
			switch (size) {
			case sizeof(int8_t): return *(int8_t *) buf > *(int64_t * )(node->value);
			case sizeof(int16_t): return *(int16_t *) buf > *(int64_t * )(node->value);
			case sizeof(int32_t): return *(int32_t *) buf > *(int64_t * )(node->value);
			case sizeof(int64_t): return *(int64_t *) buf > *(int64_t * )(node->value);
			default: return -1;
			}
		default: return -1;
		}

	case FF_OP_LT:

		switch (node->type) {
		case FF_TYPE_UINT64: return *(uint64_t *) & buf < *(uint64_t *) node->value;
		case FF_TYPE_UINT32: return *(uint32_t *) & buf < *(uint32_t *) node->value;
		case FF_TYPE_UINT16: return *(uint16_t *) & buf < *(uint16_t *) node->value;
		case FF_TYPE_UINT8: return *(uint8_t *) & buf < *(uint8_t *) node->value;
		case FF_TYPE_INT64: return *(int64_t *) & buf < *(int64_t *) node->value;
		case FF_TYPE_INT32: return *(int32_t *) & buf < *(int32_t *) node->value;
		case FF_TYPE_INT16: return *(int16_t *) & buf < *(int16_t *) node->value;
		case FF_TYPE_INT8: return *(int8_t *) & buf < *(int8_t *) node->value;

		case FF_TYPE_DOUBLE: return *(double *) &buf < *(double *) node->value;
		case FF_TYPE_STRING: return strcmp((char *) &buf, node->value) < 0;

		case FF_TYPE_UNSIGNED_BIG:
			if (size > node->vsize) { return -1; }                /* too big integer */
			switch (size) {
			case sizeof(uint8_t): return *(uint8_t *) buf < *(uint64_t * )(node->value);
			case sizeof(uint16_t): return ntohs(*(uint16_t *) buf) < *(uint64_t * )(node->value);
			case sizeof(uint32_t): return ntohl(*(uint32_t *) buf) < *(uint64_t * )(node->value);
			case sizeof(uint64_t): return ntohll(*(uint64_t *) buf) < *(uint64_t * )(node->value);
			default: return -1;
			}
		case FF_TYPE_UNSIGNED:
			if (size > node->vsize) { return -1; }                /* too big integer */
			switch (size) {
			case sizeof(uint8_t): return *(uint8_t *) buf < *(uint64_t * )(node->value);
			case sizeof(uint16_t): return *(uint16_t *) buf < *(uint64_t * )(node->value);
			case sizeof(uint32_t): return *(uint32_t *) buf < *(uint64_t * )(node->value);
			case sizeof(uint64_t): return *(uint64_t *) buf < *(uint64_t * )(node->value);
			default: return -1;
			}
		case FF_TYPE_SIGNED_BIG:
			if (size > node->vsize) { return -1; }                /* too big integer */
			switch (size) {
			case sizeof(int8_t): return *(int8_t *) buf < *(int64_t * )(node->value);
			case sizeof(int16_t): return ntohs(*(int16_t *) buf) < *(int64_t * )(node->value);
			case sizeof(int32_t): return ntohl(*(int32_t *) buf) < *(int64_t * )(node->value);
			case sizeof(int64_t): return ntohll(*(int64_t *) buf) < *(int64_t * )(node->value);
			default: return -1;
			}
		case FF_TYPE_SIGNED:
			if (size > node->vsize) { return -1; }                /* too big integer */
			switch (size) {
			case sizeof(int8_t): return *(int8_t *) buf < *(int64_t * )(node->value);
			case sizeof(int16_t): return *(int16_t *) buf < *(int64_t * )(node->value);
			case sizeof(int32_t): return *(int32_t *) buf < *(int64_t * )(node->value);
			case sizeof(int64_t): return *(int64_t *) buf < *(int64_t * )(node->value);
			default: return -1;
			}
		default: return -1;
		}

		/* For flags */
	case FF_OP_ISSET:

		switch (node->type) {
		case FF_TYPE_UNSIGNED_BIG:
			switch (size) {
			case sizeof(uint64_t):
				return (ntohll(*(uint64_t *) buf) & *(uint64_t *) node->value) ==
					*(uint64_t *)node->value;
			case sizeof(uint32_t):
				return (ntohl(*(uint32_t *) buf) & *(uint32_t *) node->value) ==
					*(uint32_t *)node->value;
			case sizeof(uint16_t):
				return (ntohs(*(uint16_t *) buf) & *(uint16_t *) node->value) ==
					*(uint16_t *)node->value;
			case sizeof(uint8_t):
				return ((*(uint8_t *) buf) & *(uint8_t *) node->value) ==
					*(uint8_t *) node->value;
			default: return -1;
			}
		case FF_TYPE_UNSIGNED:
			switch (size) {
			case sizeof(uint64_t):
				return ((*(uint64_t *) buf) & *(uint64_t *) node->value) ==
					*(uint64_t *)node->value;
			case sizeof(uint32_t):
				return ((*(uint32_t *) buf) & *(uint32_t *) node->value) ==
					*(uint32_t *)node->value;
			case sizeof(uint16_t):
				return ((*(uint16_t *) buf) & *(uint16_t *) node->value) ==
					*(uint16_t *) node->value;
			case sizeof(uint8_t):
				return ((*(uint8_t *) buf) & *(uint8_t *) node->value) ==
					*(uint8_t *) node->value;
			default: return -1;
			}
		default: return -1;
		}
		/* Compare against list */
	case FF_OP_IN: return ff_eval_node(filter, node->value, rec);

	case FF_OP_NOT:
	case FF_OP_OR:
	case FF_OP_AND:	return -1;

	}
}

ff_error_t ff_options_init(ff_options_t **poptions) {

	ff_options_t *options;

	options = malloc(sizeof(ff_options_t));

	if (options == NULL) {
		*poptions = NULL;
		return FF_ERR_NOMEM;
	}

	*poptions = options;

	return FF_OK;

}

/* release all resources allocated by filter */
ff_error_t ff_options_free(ff_options_t *options) {

	/* !!! memory cleanup */
	free(options);

	return FF_OK;

}


ff_error_t ff_init(ff_t **pfilter, const char *expr, ff_options_t *options) {

	yyscan_t scanner;
	YY_BUFFER_STATE buf;
	int parse_ret;
	ff_t *filter;

	filter = malloc(sizeof(ff_t));
	*pfilter = NULL;

	if (filter == NULL) {
		return FF_ERR_NOMEM;
	}

	filter->root = NULL;


	if (options == NULL) {
		free(filter);
		return FF_ERR_OTHER;

	}
	memcpy(&filter->options, options, sizeof(ff_options_t));

	ff_set_error(filter, "No Error.");

	ff2_lex_init(&scanner);
	buf = ff2__scan_string(expr, scanner);
	parse_ret = ff2_parse(scanner, filter);


	ff2_lex_destroy(scanner);

	/* error in parsing */
	if (parse_ret != 0) {
		*pfilter = filter;
		return FF_ERR_OTHER_MSG;
	}

	*pfilter = filter;

	return FF_OK;
}

/* matches the record agains filter */
/* returns 1 - record was matched, 0 - record wasn't matched */
int ff_eval(ff_t *filter, void *rec) {

	/* call eval node on root node */
	return ff_eval_node(filter, filter->root, rec);

}


void ff_free_node(ff_node_t* node) {

	if (node == NULL) {
		return;
	}

	ff_free_node(node->left);
	ff_free_node(node->right);

	if(node->vsize > 0) {
		free(node->value);
	}

	free(node);
}

/* release all resources allocated by filter */
ff_error_t ff_free(ff_t *filter) {

	/* !!! memory cleanup */

	if (filter != NULL) {
		ff_free_node(filter->root);
	}
	free(filter);

	return FF_OK;

}

