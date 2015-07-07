WebHost = function(par)
{
	var self = this

	// Network interface
	self.discover = function()
	{
		//debug('discover: root')
		broadcast({
			'uuid': self.uuid,
			'capabilities': [
				{
					'direction': 'input',
					'class':     'webapp',
					'client':    self.client,
					'device':    self.device,
				},
			]
		})
	}

	self.receive = function(msg)
	{
		//debug('receive: root - ' + msg.uuid + ' - ' + JSON.stringify(msg))
		if (msg.webapp && msg.webapp.name && msg.webapp.code) {
			//debug('webapp receive')
			if (msg.dst && msg.dst != self.uuid)
				return
			eval('var code = ' + msg.webapp.code + '\n' +
			     '//# sourceURL=' + msg.webapp.name)
			if (msg.command == 'add') {
                                self.add(msg.webapp.name, code, msg.webapp.title)
                        }
			if (msg.command == 'run') {
                                self.add(msg.webapp.name, code, msg.webapp.title)
                                self.run(self.apps[msg.webapp.name], msg.uuid)
                        }
		}
	}

	// Helper functions
	self.run = function(app, client) {
	        self.client = client ? client : self.uuid
		self.child = document.createElement('div')
		self.child.style.cssText =
			'position: absolute;' +
			'width:    100%;'     +
			'z-index:  0;'        +
			'top:      0;'        +
			'bottom:   0;'

		if (app.title)
			self.child.style.top       = '1.7em'

		app.proc = new app.code(self.child)
		self.par.appendChild(self.child)
		return app.proc
	}

	self.add = function(name, code, title) {
		if (self.apps[name])
			return

		self.apps[name] = {
			'name':  name,
			'code':  code,
			'title': title,
			'proc':  null,
		}

		if (title && !self.client) {
			self.toolbar.style.display = 'block'
			self.client = true
		}

		var btn = document.createElement('input')
		btn.value = name
		btn.type  = 'button'
		btn.style.cssText =
			'height: 100%;' +
			'margin: 0em .2em;'
		btn.onclick = function() {
			self.run(self.apps[name], self.uuid)
		}
		self.toolbar.appendChild(btn)
	}

	self.del = function() {
		debug('WebHost.del')
		remove(self.uuid)
		for (var name in self.apps) {
			var app = self.apps[name]
			debug('WebHost.del - ' + name + ' = ' + app + ' = ' + app.proc)
			if (app.proc && app.proc.del)
				app.proc.del()
		}
	}

	// Properties
	self.apps   = { }
	self.caps   = { }
	self.par    = par 
	self.device = 'unknown' 
	self.uuid   = listen(function(){self.discover()},
	                     function(m){self.receive(m)})

	// Initalize
	self.toolbar = document.createElement('div')
	self.toolbar.style.cssText =
		'display:       none;'              +
		'position:      relative;'          +
		'border-bottom: solid 1px #dddddd;' +
		'padding:       0.3em 0.5em;'       +
		'height:        1.2em;'             +
		'z-index:       2;'                 +
		'background:    #eeeeee;'
	self.par.appendChild(self.toolbar)

	// Test
	//self.add('hi', function(elem){
	//	elem.innerHTML = '<div>Hello</div>'
	//})
}
