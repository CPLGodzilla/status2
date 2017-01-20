# Gitlab2Discord Webhook Bridge

A webhook bridge for posting Gitlab webhooks to Discord.

It handles:
+ Push
+ Tag push
+ Comments
+ Issues
+ Notes
+ Merge Requests
+ Wiki Pages
+ Pipelines
+ Builds

Also partial support for system hook (push/push_tag) events

## Requires

+ mongoose
+ curl
+ premake 4 (for generating project files)

## Configuration

All you need to do is:
1. Rename `config.example.json` to `config.json` and open it.
1. Set the port you want to use. (default is `4321`)
1. Generate the webhook url in Discord
	+ Go to your servers settings
	+ Click on the "webhooks" tab
	+ Click the "Create Webhook" button
	+ Copy the "Webhook URL"
1. Paste the Webhook URL into the config.json

## Install

1. `git clone https://github.com/Xbetas/gitlab2discord_webhook_bridge.git ~/webhook_bridge`
1. `cd ~/webhook_bridge/bin`
1. `premake4 gmake`
1. `make config=release`
1. Place the contents of `bin` anywhere you'd like to have them.

## Usage

1. Start the Webhook Bridge
1. Add a webhook to your Gitlab project
	+ Add URL `http://localhost:4321/` (replace localhost with correct ip if bridge is not running on the same machine as gitlab)
	+ Check all the triggers you want
	+ No Secret token required
1. Use Gitlab and watch your Discord channel!

Note: The bridge will also notify of unsupported events, though these happen only with system hooks
