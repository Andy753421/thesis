// TODO:
//   We need to serialize everything as a string
//   before sending it to Java because Android
//   doesn't support passing functions or complex
//   objects as parameters and return values.

function connect()
{
	android.connect();
}

function listen(discover, receive) 
{
	return android.listen(discover, receive);
}

function remove(handle) 
{
	android.remove(handle);
}

function broadcast(mesg, uuid)
{
	android.broadcast(mesg, uuid);
}
