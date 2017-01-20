#include <stdio.h>
#include <string.h>
#include <fstream>
#include <iostream>

using namespace std;

#include <curl/curl.h>

#include "mongoose.h"
#include "picojson.h"

//url to post webhooks to (gets filled by config reading in main)
string post_url = "";

string create_discord_webhook(string message)
{
	string result = "{\"username\":\"Gitlab\",\"avatar_url\":\"https://about.gitlab.com/images/new_logo/A.jpg\",\"content\":\"";
	result.append(message);
	result.append("\"}");

	//cout << result << endl;

	return result;
}

string convert_newlines(string mess)
{
	string ret = mess;

	//convert and remove certain symbols for titles and long descriptions
	string::size_type pos = 0; // Must initialize
	while ( ( pos = ret.find("\n",pos) ) != string::npos )
	{
		ret.erase( pos, 1 );
		ret.insert( pos, "\\n");
	}
	pos = 0;
	while ( ( pos = ret.find('"',pos) ) != string::npos )
	{
		ret.erase( pos, 1 );
		ret.insert( pos, "\'\'");
	}
	pos = 0;
	while ( ( pos = ret.find("\r",pos) ) != string::npos )
	{
		ret.erase( pos, 1 );
	}

	return ret;

}

void post_webhook_to_discoord(string message, string url)
{
	//use curl to post a transformed webhook to url
	CURL *curl;
	CURLcode result;

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

		string res = message;

		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, res.c_str());

		result = curl_easy_perform(curl);
		if(result != CURLE_OK)
		{
			printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
		}

		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
}

//---------------------hook handlers---------------VVVVV
string handle_push(picojson::value& v)
{
	string project_name = v.get<picojson::object>()["project"].get<picojson::object>()["name"].to_str();

	string project_branch = v.get<picojson::object>()["ref"].to_str();
	//remove things before '/'
	string::size_type pos = 0; // Must initialize
	while ( ( pos = project_branch.find("/",pos) ) != string::npos )
	{
		project_branch.erase( 0, pos+1 );
	}

	int num_commits = stoi(v.get<picojson::object>()["total_commits_count"].to_str());

	string id[num_commits];
	string authors[num_commits];
	string message[num_commits];

	for(int i = 0; i < num_commits; i++)
	{
		id[i] = v.get<picojson::object>()["commits"].get<picojson::array>()[i].get<picojson::object>()["id"].to_str();
		id[i].resize(7);
		authors[i] = v.get<picojson::object>()["commits"].get<picojson::array>()[i].get<picojson::object>()["author"].get<picojson::object>()["name"].to_str();
		message[i] = convert_newlines(v.get<picojson::object>()["commits"].get<picojson::array>()[i].get<picojson::object>()["message"].to_str());

	}

	string hookpost = "***[" + project_name + ":" + project_branch + "]*** \\n";
	if(num_commits > 1) { hookpost += to_string(num_commits) + " new commits:\\n"; }
	else { hookpost += "1 new commit\\n"; }

	for(int i = 0; i < num_commits; i++)
	{
		hookpost += "`" + id[i] + "` by **" + authors[i] + "**: \\n" + message[i] + "\\n";
	}

	return hookpost;
}

string handle_push_tag(picojson::value& v)
{
	string project_name = v.get<picojson::object>()["project"].get<picojson::object>()["name"].to_str();

	string project_branch = v.get<picojson::object>()["ref"].to_str();
	//remove things before '/'
	string::size_type pos = 0; // Must initialize
	while ( ( pos = project_branch.find("/",pos) ) != string::npos )
	{
		project_branch.erase( 0, pos+1 );
	}

	string id = v.get<picojson::object>()["checkout_sha"].to_str();
	id.resize(7);
	string user = v.get<picojson::object>()["user_name"].to_str();

	string message = convert_newlines(v.get<picojson::object>()["message"].to_str());

	bool deleted = (message == "null") ? false : true;

	string hookpost = "***[" + project_name + ":" + project_branch + "]*** \\n";
	if(deleted) hookpost += "1 new tag\\n`" + id + "` by **" + user + "**: \\n" + message + "\\n";
	else hookpost += "1 tag deleted\\n";

	return hookpost;
}

string handle_issue(picojson::value& v)
{
	string user = v.get<picojson::object>()["user"].get<picojson::object>()["name"].to_str();

	string project_name = v.get<picojson::object>()["project"].get<picojson::object>()["name"].to_str();

	string title = convert_newlines(v.get<picojson::object>()["object_attributes"].get<picojson::object>()["title"].to_str());
	string description = convert_newlines(v.get<picojson::object>()["object_attributes"].get<picojson::object>()["description"].to_str());
	string action = v.get<picojson::object>()["object_attributes"].get<picojson::object>()["action"].to_str();

	string i_id = v.get<picojson::object>()["object_attributes"].get<picojson::object>()["id"].to_str();
	string i_url = v.get<picojson::object>()["object_attributes"].get<picojson::object>()["url"].to_str();

	string due_date;
	string assignee_name;
	string assignee_username;;

	bool has_due_date = false;
	if(v.get<picojson::object>()["object_attributes"].get<picojson::object>()["due_date"].to_str() != "null") {
		due_date = v.get<picojson::object>()["object_attributes"].get<picojson::object>()["due_date"].to_str();
		has_due_date = true;
	}
	bool has_assignee = false;
	if(v.get<picojson::object>()["assignee"].to_str() != "null") {
		assignee_name = v.get<picojson::object>()["assignee"].get<picojson::object>()["name"].to_str();
		assignee_username = v.get<picojson::object>()["assignee"].get<picojson::object>()["username"].to_str();
		has_assignee = true;
	}

	string hookpost = "***[" + project_name + "]*** \\n";
	if (action == "open")			hookpost += "`Issue #" + i_id + "` opened by **" + user + "**:";
	else if (action == "reopen")	hookpost += "`Issue #" + i_id + "` reopened by **" + user + "**:";
	else if (action == "update")	hookpost += "`Issue #" + i_id + "` updated by **" + user + "**:";
	else if (action == "close")		hookpost += "`Issue #" + i_id + "` closed by **" + user + "**:";

	hookpost += "\\n__" + i_url + "__";
	if(has_assignee) hookpost += "\\nAssigned to: **" + assignee_name + "** *(" + assignee_username + ")*";
	if(has_due_date) hookpost += "\\nDue Date: `" + due_date + "`";

	if(description == "null") hookpost += "\\n```markdown\\n**" + title + "**```";
	else hookpost += "\\n```markdown\\n**" + title + "**\\n\\n" + description + "\\n```";

	return hookpost; //"Issue Hook not implemented yet!";
}

string handle_note(picojson::value& v)
{
	string user = v.get<picojson::object>()["user"].get<picojson::object>()["name"].to_str();
	string username = v.get<picojson::object>()["user"].get<picojson::object>()["username"].to_str();
	string project_name = v.get<picojson::object>()["project"].get<picojson::object>()["name"].to_str();

	string note = convert_newlines(v.get<picojson::object>()["object_attributes"].get<picojson::object>()["note"].to_str());
	string note_type = v.get<picojson::object>()["object_attributes"].get<picojson::object>()["noteable_type"].to_str();

	string n_id = v.get<picojson::object>()["object_attributes"].get<picojson::object>()["id"].to_str();
	string n_url = v.get<picojson::object>()["object_attributes"].get<picojson::object>()["url"].to_str();

	string hookpost = "***[" + project_name + "]*** ";

	hookpost += "\\n__" + n_url + "__";

	if(note_type == "Commit") {
		string commit_id = v.get<picojson::object>()["object_attributes"].get<picojson::object>()["commit_id"].to_str();
		commit_id.resize(7);
		hookpost += "\\n**" + user + "** *(" + username + ")* commented on commit`" + commit_id + "`:";
	}
	else if(note_type == "MergeRequest") {
		string source = v.get<picojson::object>()["merge_request"].get<picojson::object>()["source_branch"].to_str();
		string target = v.get<picojson::object>()["merge_request"].get<picojson::object>()["target_branch"].to_str();
		hookpost += "\\n**" + user + "** *(" + username + ")* commented on merge request `" + source + "->" + target + "`:";
	}
	else if(note_type == "Issue") {
		string i_id = v.get<picojson::object>()["issue"].get<picojson::object>()["id"].to_str();
		hookpost += "\\n**" + user + "** *(" + username + ")* commented on `Issue #" + i_id + "`:";
	}
	else if(note_type == "Snippet") {
		string s_name = v.get<picojson::object>()["snippet"].get<picojson::object>()["title"].to_str();
		hookpost += "\\n**" + user + "** *(" + username + ")* commented on snippet `" + s_name + "`:";
	}

	hookpost += "\\n```markdown\\n" + note + "\\n```";

	return hookpost; //"Note Hook not implemented yet!";
}

string handle_merge_request(picojson::value& v)
{
	string user = v.get<picojson::object>()["user"].get<picojson::object>()["name"].to_str();
	string username = v.get<picojson::object>()["user"].get<picojson::object>()["username"].to_str();

	string merge_title = convert_newlines(v.get<picojson::object>()["object_attributes"].get<picojson::object>()["title"].to_str());

	string source_name = v.get<picojson::object>()["object_attributes"].get<picojson::object>()["source"].get<picojson::object>()["name"].to_str();
	string source_branch = v.get<picojson::object>()["object_attributes"].get<picojson::object>()["source_branch"].to_str();
	string target_name = v.get<picojson::object>()["object_attributes"].get<picojson::object>()["source"].get<picojson::object>()["name"].to_str();
	string target_branch = v.get<picojson::object>()["object_attributes"].get<picojson::object>()["target_branch"].to_str();

	//string title = v.get<picojson::object>()["object_attributes"].get<picojson::object>()["title"].to_str();
	string description = convert_newlines(v.get<picojson::object>()["object_attributes"].get<picojson::object>()["description"].to_str());
	string action = v.get<picojson::object>()["object_attributes"].get<picojson::object>()["action"].to_str();

	string hookpost = "***Merge Request*** \\n";
	hookpost += "`[" + source_name + ":" + source_branch + "]->[" + target_name + ":" + target_branch + "]` \\n";

	if(action == "open")			hookpost += "opened by **" + user + "** (*" + username + "*):";
	else if( action == "reopen")	hookpost += "reopened by **" + user + "** (*" + username + "*):";
	else if( action == "merge")		hookpost += "merged by **" + user + "** (*" + username + "*):";
	else if( action == "update")	hookpost += "updated by **" + user + "** (*" + username + "*):";
	else if(action == "close")		hookpost += "closed by **" + user + "** (*" + username + "*):";

	if(description == "null") hookpost += "\\n```markdown\\n**" + merge_title + "**```";
	else hookpost += "\\n```markdown\\n**" + merge_title + "**\\n\\n" + description + "\\n```";

	return hookpost;
}

string handle_wiki_page(picojson::value& v)
{
	string user = v.get<picojson::object>()["user"].get<picojson::object>()["name"].to_str();
	string username = v.get<picojson::object>()["user"].get<picojson::object>()["username"].to_str();

	string project_name = v.get<picojson::object>()["project"].get<picojson::object>()["name"].to_str();

	string title = convert_newlines(v.get<picojson::object>()["object_attributes"].get<picojson::object>()["title"].to_str());
	string content = convert_newlines(v.get<picojson::object>()["object_attributes"].get<picojson::object>()["content"].to_str());

	string action = v.get<picojson::object>()["object_attributes"].get<picojson::object>()["action"].to_str();
	string url = v.get<picojson::object>()["wiki"].get<picojson::object>()["web_url"].to_str();


	string hookpost = "***[" + project_name + "]*** \\n";
	hookpost += url + "\\n";
	if(action == "create")			hookpost += "Wiki page `" + title + "` created by **" + user + "** *(" + username + ")*:";
	else if( action == "update")	hookpost += "Wiki page `" + title + "` updated by **" + user + "** *(" + username + ")*:";

	if(content == "null") hookpost += "\\n```markdown\\n**" + title + "**```";
	else hookpost += "\\n```markdown\\n**" + title + "**\\n\\n" + content + "\\n```";

	return hookpost; // "Wiki Page Hook not implemented yet!";
}

string handle_pipeline(picojson::value& v)
{
	string user = v.get<picojson::object>()["user"].get<picojson::object>()["name"].to_str();

	string project_name = v.get<picojson::object>()["project"].get<picojson::object>()["name"].to_str();

	string p_id = v.get<picojson::object>()["object_attributes"].get<picojson::object>()["id"].to_str();
	string p_status = v.get<picojson::object>()["object_attributes"].get<picojson::object>()["status"].to_str();
	string p_duration = v.get<picojson::object>()["object_attributes"].get<picojson::object>()["duration"].to_str();
	p_duration.resize(3);

	string hookpost = "***[" + project_name + "]*** ";

	hookpost += " **Pipeline #" + p_id + "** `" + p_status + "` ";
	if(p_duration != "nul") hookpost += p_duration;

	return hookpost; //"Pipeline Hook not implemented!";
}

string handle_build(picojson::value& v)
{
	string project_name = v.get<picojson::object>()["project_name"].to_str();

	string b_name = v.get<picojson::object>()["build_name"].to_str();
	string b_stage = v.get<picojson::object>()["build_stage"].to_str();
	string b_status = v.get<picojson::object>()["build_status"].to_str();
	string b_duration = v.get<picojson::object>()["build_duration"].to_str();
	b_duration.resize(3);

	string hookpost = "***[" + project_name + "]*** ";

	hookpost += " Build **" + b_name + "**:*[" + b_stage + "]* `" + b_status + "` ";
	if(b_duration != "nul") hookpost += b_duration;

	return hookpost; //"Build Hook not implemented yet!";
}
//----------------------------------------------

string convert_request_to_webhook(picojson::value& v, string http_request_type)
{

	if(http_request_type == "Push Hook")				{ return ":cookie: " +				handle_push(v); }
	else if(http_request_type == "Tag Push Hook")		{ return ":record_button: " +		handle_push_tag(v); }
	else if(http_request_type == "Issue Hook")			{ return ":notepad_spiral:" +		handle_issue(v); }
	else if(http_request_type == "Note Hook")			{ return ":speech_balloon: " +		handle_note(v); }
	else if(http_request_type == "Merge Request Hook")	{ return ":gift: " +				handle_merge_request(v); }
	else if(http_request_type == "Wiki Page Hook")		{ return ":book: " +				handle_wiki_page(v); }
	else if(http_request_type == "Pipeline Hook")		{ return ":traffic_light: " +		handle_pipeline(v); }
	else if(http_request_type == "Build Hook")			{ return ":construction_site: " +	handle_build(v); }

	//you could handle system hooks like this if you'd like
	else if(http_request_type == "System Hook") {
		if(v.get<picojson::object>()["event_name"].to_str() == "push")		{ return ":cookie: (System) " + handle_push(v); }
		else if(v.get<picojson::object>()["event_name"].to_str() == "push_tag")	{ return ":record_button: (System) " + handle_push_tag(v); }
		else { return ":octagonal_sign: A unsupported system hook happened!"; }
	}
	else
	{
		return ":octagonal_sign: A UNSUPPORTED REQUEST HAPPENED! `" + http_request_type + "`";
	}
}

//---------web server below--------------VVVVV

static void ev_handler(struct mg_connection *nc, int ev, void *p)
{
	if(ev == MG_EV_HTTP_REQUEST) {
		struct http_message* hm = (struct http_message*)p;

		//Get the event type
		string h_event = mg_get_http_header(hm, "X-Gitlab-Event")->p;
		h_event.resize(mg_get_http_header(hm, "X-Gitlab-Event")->len);

		//parse json from the gitlab message
		picojson::value v;
		string json = string(hm->body.p);
		picojson::parse(v, json);

		//create a discord-compatible webhook and post it to the link in the config
		string hookpost = convert_request_to_webhook(v, h_event);
		post_webhook_to_discoord(create_discord_webhook(hookpost), post_url);

		//send back a "request success (code 200)"
		mg_send_head(nc, 200, hm->message.len, "Content-Type: text/plain");
		mg_printf(nc, "%.*s", (int)hm->message.len, hm->message.p);
	}
}

int main()
{
	//config reading code
	ifstream file("config.json");
	string json_config;
	getline(file, json_config);

	picojson::value config;
	picojson::parse(config, json_config);

	//we'll just blindly trust that the config is in order for now
	string port = config.get<picojson::object>()["port"].to_str();
	post_url = config.get<picojson::object>()["webhook_url"].to_str();

	//set up mongoose http server
	struct mg_mgr mgr;
	struct mg_connection *nc;

	mg_mgr_init(&mgr, NULL);
	printf("Starting web server on port %s\n", port.c_str());

	nc = mg_bind(&mgr, port.c_str(), ev_handler);
	if(nc == NULL)
	{
		printf("Failed to create listener\n");
		return 1;
	}

	//Set up http server parameters
	mg_set_protocol_http_websocket(nc);

	//Main loop
	for(;;)
	{
		mg_mgr_poll(&mgr, 1000);
	}

	//free mongoose
	mg_mgr_free(&mgr);

	return 0;
}
