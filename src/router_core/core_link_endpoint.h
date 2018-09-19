/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef qd_router_core_endpoint_types
#define qd_router_core_endpoint_types 1

typedef struct qdrc_endpoint_t qdrc_endpoint_t;

#include "router_core_private.h"
#endif

#ifndef qd_router_core_endpoint
#define qd_router_core_endpoint 1


/**
 * Event - An attach for a new core-endpoint link has arrived
 *
 * @Param bind_context The opaque context provided in the mobile address binding
 * @Param endpoint A new endpoint object for the link.  If the link is accepted, this
 *                 object must be stored for later use.
 * @Param link_context [out] Handler-provided opaque context to be associated with the endpoint
 * @Param error [out] Error indication which may be supplied if the link is rejected
 * @Return True if the link is to be accepted, False if the link should be rejected and detached
 */
typedef bool (*qdrc_first_attach_t) (void             *bind_context,
                                     qdrc_endpoint_t  *endpoint,
                                     void            **link_context,
                                     qdr_error_t     **error);

/**
 * Event - The attachment of a link initiated by the core-endpoint was completed
 *
 * Note that core-endpoint incoming links are _not_ provided credit by the core.  It
 * is the responsibility of the core-endpoint to supply credit at the appropriate time
 * by calling qdrc_endpoint_flow_CT.
 *
 * @Param link_context The opaque context supplied in the call to qdrc_endpoint_create_link_CT
 */
typedef bool (*qdrc_second_attach_t) (void *link_context);

/**
 * Event - Credit/Drain status for an outgoing core-endpoint link has changed
 *
 * @Param link_context The opaque context associated with the endpoint link
 * @Param available_credit The number of deliveries that may be sent on this link
 * @Param drain True iff the peer receiver is requesting that the credit be drained
 */
typedef void (*qdrc_flow_t) (void *link_context,
                             int   available_credit,
                             bool  drain);

/**
 * Event - The settlement and/or disposition of a delivery has been updated
 *
 * @Param link_context The opaque context associated with the endpoint link
 * @Param delivery The delivery object experiencing the change
 * @Param settled True iff the delivery has been settled by the peer
 * @Param disposition The disposition of the delivery (PN_[ACCEPTED|REJECTED|MODIFIED|RELEASED])
 */
typedef void (*qdrc_update_t) (void           *link_context,
                               qdr_delivery_t *delivery,
                               bool            settled,
                               uint64_t        disposition);

/**
 * Event - A message transfer has arrived over a core-endpoint link
 *
 * Note that a message may arrive in multiple transfers.  If a complete message is
 * required, this handler _must_ use the qd_message_receive_complete method on the
 * message to ensure the message has been completely received.
 *
 * @Param link_context The opaque context associated with the endpoint link
 * @Param delivery Pointer to the delivery object for the transfer
 * @Param message Pointer to the message being transferred
 */
typedef void (*qdrc_transfer_t) (void           *link_context,
                                 qdr_delivery_t *delivery,
                                 qd_message_t   *message);

/**
 * Event - A core-endpoint link has been detached
 *
 * @Param link_context The opaque context associated with the endpoint link
 * @Param error The error information that came with the detach or 0 if no error
 */
typedef void (*qdrc_detach_t) (void        *link_context,
                               qdr_error_t *error);


typedef struct qdrc_endpoint_desc_t {
    qdrc_first_attach_t   on_first_attach;
    qdrc_second_attach_t  on_second_attach;
    qdrc_flow_t           on_flow;
    qdrc_update_t         on_update;
    qdrc_transfer_t       on_transfer;
    qdrc_detach_t         on_detach;
} qdrc_endpoint_desc_t;


/**
 * Bind a core-endpoint to a mobile address
 *
 * This will create a mobile address in the core's address table that is bound
 * to the core-endpoint.  Any incoming links with terminus addresses that match
 * this address will be directed to the core-endpoint for handling.
 *
 * @Param core Pointer to the core object
 * @Param address The address as a null-terminated character string
 * @Param phase The phase of the address (typically '0')
 * @Param desc The descriptor for this core endpoint containing all callbacks
 * @Param bind_context An opaque context that will be included in the call to on_first_attach
 */
void qdrc_endpoint_bind_mobile_address_CT(qdr_core_t           *core,
                                          const char           *address,
                                          char                  phase,
                                          qdrc_endpoint_desc_t *desc,
                                          void                 *bind_context);


/**
 * Create a new link terminated by the core-endpoint
 *
 * Initiate the attachment of a new link outbound to a remote node.  The link will
 * be known to be fully attached when the on_second_attach callback is invoked.
 *
 * @Param core Pointer to the core object
 * @Param conn Pointer to the connection object over which the link will be created
 * @Param dir The direction of the link: QD_INCOMING or QD_OUTGOING
 * @Param local_source The source terminus of the link - must be included for incoming links
 * @Param local_target The target terminus of the link - must be included for outgoing links
 * @Param desc The descriptor for this core endpoint containing all callbacks
 * @Param link_context An opaque context that will be included in the calls to the callbacks
 * @Return Pointer to a new qdrc_endpoint_t for tracking the link
 */
qdrc_endpoint_t *qdrc_endpoint_create_link_CT(qdr_core_t           *core,
                                              qdr_connection_t     *conn,
                                              qd_direction_t        dir,
                                              qdr_terminus_t       *local_source,
                                              qdr_terminus_t       *local_target,
                                              qdrc_endpoint_desc_t *desc,
                                              void                 *link_context);

/**
 * Accessors for data held by the endpoint
 *
 *  - The link's direction of delivery flow
 *  - The link's connection
 *
 * @Param endpoint Pointer to an endpoint object
 * @Return The requested information (or 0 if not present)
 */
qd_direction_t    qdrc_endpoint_get_direction_CT(const qdrc_endpoint_t *endpoint);
qdr_connection_t *qdrc_endpoint_get_connection_CT(qdrc_endpoint_t *endpoint);

/**
 * Issue credit and control drain for an incoming link
 *
 * @Param core Pointer to the core object
 * @Param endpoint Pointer to an endpoint object
 * @Param credit_added Number of credits to grant to the sender
 * @Param drain Indication that you want the sender to drain available credit
 */
void qdrc_endpoint_flow_CT(qdr_core_t *core, qdrc_endpoint_t *endpoint, int credit_added, bool drain);

/**
 * Send a message via an outgoing link
 *
 * @Param core Pointer to the core object
 * @Param endpoint Pointer to an endpoint object
 * @Param delivery A delivery containing a message that is to be sent on the link
 * @Param presettled True iff the delivery is to be presettled.  If presettled, no further action
 *                   will be needed for the delivery.  If not presettled, an on_update event for
 *                   the delivery should be expected.
 */
void qdrc_endpoint_send_CT(qdr_core_t *core, qdrc_endpoint_t *endpoint, qdr_delivery_t *delivery, bool presettled);

/**
 * Settle a received delivery with a specified disposition
 *
 * @Param core Pointer to the core object
 * @Param delivery Pointer to a received delivery
 * @Param disposition The desired disposision of the delivery (use PN_[ACCEPTED|REJECTED|MODIFIED|RELEASED])
 */
void qdrc_endpoint_settle_CT(qdr_core_t *core, qdr_delivery_t *delivery, uint64_t disposition);

/**
 * Detach a link attached to the core-endpoint
 *
 * @Param core Pointer to the core object
 * @Param endpoint Pointer to an endpoint object
 * @Param error An error indication or 0 for no error
 */
void qdrc_endpoint_detach_CT(qdr_core_t *core, qdrc_endpoint_t *endpoint, qdr_error_t *error);


//=====================================================================================
// Private functions, not part of the API
//=====================================================================================

bool qdrc_endpoint_do_bound_attach_CT(qdr_core_t *core, qdr_address_t *addr, qdr_link_t *link, qdr_error_t **error);

#endif
