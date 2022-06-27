#!/usr/bin/env python3

import datetime
import logging
import asyncio
import re
from tkinter.messagebox import RETRY
import aiocoap.resource as resource
import aiocoap
from database_tools import *

class RowMeanData(resource.Resource):
    def __init__(self):
        super().__init__()

    async def render_put(self, request):
        MessageParsing().row_mean_data_parsing(request.payload)
        return aiocoap.Message(code=aiocoap.CHANGED, payload=request.payload)

class UserPayload(resource.ObservableResource):
    # Initialize the last timestamp threshold
    last_timestamp_threshold = UserRequestsDBTools().get_latest_timestamp_threshold_request()
    last_timestamp_control = UserRequestsDBTools().get_latest_timestamp_control_request()
    retry = 0
    if last_timestamp_threshold == '':
        last_timestamp_threshold = '1970-06-19 21:00:31'
    if last_timestamp_control == '':
        last_timestamp_threshold = '1970-06-19 21:00:31'

    def __init__(self):
        super().__init__()
        self.payload = ''
        self.notify()

    def notify(self):
        if self.retry == 0:
            # get the new timestamp
            ret_threshold_timestamp = UserRequestsDBTools().get_next_threshold_request_timestamp(self.last_timestamp_threshold)
            ret_control_timestamp = UserRequestsDBTools().get_next_control_request_timestamp(self.last_timestamp_control)
            # if there is new timestamp, it means we have new request
            if ret_threshold_timestamp != '':
                # update the latest timestamp for threshold requests
                self.last_timestamp_threshold = ret_threshold_timestamp
                self.payload = UserRequestsDBTools().create_user_threshold_config_message(self.last_timestamp_threshold)
                self.updated_state()
                self.retry = 2
            elif ret_control_timestamp != '':
                # update the latest timestamp for threshold requests
                self.last_timestamp_control = ret_control_timestamp
                self.payload = UserRequestsDBTools().create_user_control_config_message(self.last_timestamp_control)
                print(self.payload)
                self.updated_state()
                self.retry = 2
            else:
                # WIP: try to empty payload after update state to see if observe renew gets this empty response
                self.payload = ''
        elif self.retry > 0:
            self.updated_state()
            self.retry -= 1
        asyncio.get_event_loop().call_later(2, self.notify)

    async def render_get(self, request):
        return aiocoap.Message(payload=self.payload)
            
# logging setup

logging.basicConfig(level=logging.INFO)
logging.getLogger("coap-server").setLevel(logging.DEBUG)

async def main():
    # Resource tree creation
    root = resource.Site()

    root.add_resource(['.well-known', 'core'],
            resource.WKCResource(root.get_resources_as_linkheader))
    root.add_resource(['rowmeandata'], RowMeanData())
    root.add_resource(['userpayload'], UserPayload())
    await aiocoap.Context.create_server_context(root)

    # Run forever
    await asyncio.get_running_loop().create_future()

if __name__ == "__main__":
    asyncio.run(main())
