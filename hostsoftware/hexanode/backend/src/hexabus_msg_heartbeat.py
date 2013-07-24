#!/usr/bin/python3

import json
import argparse
import hashlib
import hmac
import http.client
import ssl
import urllib.parse
import base64
import io
import tempfile
import sys
import traceback
import subprocess

global device_id
global signing_key
global api_url

def sign_message(message):
	mac = hmac.new(bytes(signing_key, 'ascii'), digestmod=hashlib.sha1)
	mac.update(message)
	return mac.hexdigest()

def http_request(method, url, body):
	message = bytes(json.dumps(body), 'utf-8')
	signature = sign_message(message)

	url_parts = urllib.parse.urlparse(url, allow_fragments=False)
	context = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
	context.verify_mode = ssl.CERT_NONE
	client = http.client.HTTPSConnection(url_parts.hostname, url_parts.port, context=context)
	client.set_debuglevel(1)
	client.request(method, url_parts.path, message, {
		'X-Digest': signature,
		'X-Version': '1.0',
		'Accept': 'application/json',
		'Content-Type': 'application/json'
	})
	response = client.getresponse()
	if response.status != 200:
		print("Request failed with HTTP status {0}".format(response.status))
		exit(2)
	return json.loads(response.read().decode())


def perform_heartbeat():
	message = {
		'firmware': {
			'version': '0.6.0-1',
			'releasetime': '20120524_1158',
			'build': 'f0ba69e4fea1d0c411a068e5a19d0734511805bd',
			'tag': 'flukso-2.0.3-rc1-19-gf0ba69e'
		}
	}
	return http_request("POST", api_url + "/device/" + device_id, message)

def download_upgrade_package(target):
#	content = base64.decodebytes(http_request("GET", api_url + "/firmware/" + device_id, {}))
	content = b'#!/bin/sh\n\ndate\n'
	target.write(content)
	target.flush()

if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	parser.add_argument("--url")
	parser.add_argument("--device")
	parser.add_argument("--key")
	args = parser.parse_args()
	global device_id
	device_id = args.device
	global signing_key
	signing_key = args.key
	global api_url
	api_url = args.url
	try:
		status = perform_heartbeat()
#		if status["upgrade"] != 0:
		with tempfile.NamedTemporaryFile(prefix="update_", suffix=".sh") as target_file:
			download_upgrade_package(target_file)
			success = subprocess.call(["/bin/sh", target_file.name]) == 0
			if success:
				http_request("POST", api_url + "/event/105", { 'device': device_id })
			else:
				http_request("POST", api_url + "/event/106", { 'device': device_id })

	except Exception as e:
		print("Heartbeat failed:")
		print(e)
		traceback.print_tb(sys.exc_info()[2])
		exit(10)
