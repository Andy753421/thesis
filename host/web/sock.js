var sock_local   = false // local connections only
var sock_clients = {}    // local client list
var sock_timer   = 0     // window timer id

// Helper functions
function guid()
{
	var s=[]
	for (var i = 0; i < 8; i++)
		s[i] = Math.floor((1+Math.random())*0x10000)
			.toString(16).substring(1);
	return s[0]+s[1]+'-'+s[2]+'-'+s[3]+'-'+s[4]+'-'+s[5]+s[6]+s[7];
}

// Android interface
function android_callback(text)
{
	console.log("android_callback: " + text);
	var obj = JSON.parse(text);
	for (var key in sock_clients)
		if (sock_clients[key].receive)
			sock_clients[key].receive(obj);
}

// API functions
function listen(discover, receive) 
{
	var id = guid()
	sock_clients[id] = {
		discover: discover,
		receive:  receive,
	};
	if (!sock_timer) {
		sock_timer = window.setInterval(function() {
			for (var key in sock_clients)
				sock_clients[key].discover();
		}, 1000);
	}
	return id;
}

function remove(id) 
{
	delete sock_clients[id]
}

function broadcast(mesg, uuid)
{
	if (uuid)
		mesg.uuid = uuid
	for (var key in sock_clients) {
		if (!mesg.uuid || mesg.uuid != key)
			if (sock_clients[key].receive)
				sock_clients[key].receive(mesg);
	}
	if (!sock_local && typeof android !== 'undefined') {
		var text = JSON.stringify(mesg);
		android.broadcast(text);
	}
}
