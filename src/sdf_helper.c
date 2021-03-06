/*
 * SDF (Self-Describing Format) Software Library
 * Copyright (c) 2014-2016, SDF Development Team
 *
 * Distributed under the terms of the BSD 3-clause License.
 * See the LICENSE file for details.
 */

#include <string.h>
#include <sdf.h>
#include "sdf_helper.h"
#include "stack_allocator.h"


int sdf_helper_read_data(sdf_file_t *h, sdf_block_t *b)
{
    int i;
    sdf_block_t *block;

    if (!h->stack_handle){
      sdf_stack_init(h);
    }

    for (i = 0; i < b->n_ids; i++) {
        // Fill in derived components which are not already cached
        if (b->must_read[i]) {
            block = sdf_find_block_by_id(h, b->variable_ids[i]);
            if (block && !block->data) {
                sdf_block_set_array_section(block, b->ndims,
                        b->array_starts, b->array_ends, b->array_strides);
                sdf_helper_read_data(h, block);
            }
        }
    }

    if (b->blocktype == SDF_BLOCKTYPE_PLAIN_DERIVED
            || b->blocktype == SDF_BLOCKTYPE_POINT_DERIVED) {

        // Allocate derived variable data if required
        if (!b->data && !b->dont_allocate) {
            // First fix up array dimensions. This should be moved into the
            // metadata setup.
            sdf_block_t *mesh;
            if (b->mesh_id)
                mesh = sdf_find_block_by_id(h, b->mesh_id);
            else
                mesh = b->subblock;
            b->ndims = mesh->ndims;
            memcpy(b->local_dims, mesh->local_dims,
                   b->ndims * sizeof(*b->local_dims));

            if (b->blocktype == SDF_BLOCKTYPE_POINT_DERIVED) {
                b->nelements_local = mesh->dims[0];
            } else {
                b->nelements_local = 1;
                for (i=0; i < b->ndims; i++) {
                    if (!b->station_id && b->stagger == SDF_STAGGER_CELL_CENTRE)
                        b->local_dims[i]--;
                    b->nelements_local *= b->local_dims[i];
                }
            }

            if (!b->datatype_out)
                b->datatype_out = mesh->datatype_out;

            sdf_stack_alloc(h, b);
        }

        // Execute callback to fill in the derived variable
        if (b->populate_data) b->populate_data(h, b);
    }

    sdf_stack_alloc(h, b);

    h->current_block = b;
    return sdf_read_data(h);
}
