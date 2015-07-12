// Controls
Virtual = function(name, klass, par)
{
	var self = this
	
	// Help functions
	self.startTransfer = function(uuid) {
		//debug('Virtual.startTransfer: ' + uuid)
		self.time = new Date().getTime()
		self.host = uuid
		broadcast({
			'dst':     uuid,
			'command': 'run',
			'webapp':  {
				'name': self.name,
				'code': self.klass.toString(),
			},
		}, self.uuid)
	}
	self.finishTransfer = function(uuid) {
		//debug('Virtual.finishTransfer: ' + uuid)
		self.time = new Date().getTime()
		if (self.box.parentNode == self.par) {
			if (self.hide) {
				self.save = self.hide.style.display 
				self.hide.style.display = 'none'
			}
			self.par.removeChild(self.box)
		}
	}
	self.abortTransfer = function(uuid) {
		debug('Virtual.abortTransfer: ' + uuid)
		if (self.hide)
			self.hide.style.display = self.save
		self.par.appendChild(self.box)
		self.host = undefined
	}

	// Network interface
	self.discover = function() {
		debug('polling ' + self.host + ', ' + new Date().getTime() + ' - ' + self.time)
		if ((new Date().getTime() - self.time) > 1000)
			self.abortTransfer(self.host)
	}

	self.receive = function(msg) {
		if (msg && msg.uuid && msg.capabilities) {
			for (var i in msg.capabilities) {
				var caps = msg.capabilities[i];
				if (caps.direction == 'input'  &&
				    caps.class     == 'webapp') {
					if (self.want && caps.device != self.want) {
						//debug('want mismatch: ' + self.want + ' == ' +
						//		JSON.stringify(caps))
						return
					}
					if (!caps.client && !self.host)
						self.startTransfer(msg.uuid)
					if (caps.client == self.uuid)
						self.finishTransfer(msg.uuid)
					//debug('transfer: ' + self.uuid + ' == ' + JSON.stringify(caps))
				}
			}
		}
	}

	self.del = function() {
		debug('Virtual.del')
		remove(self.uuid)
	}

	// Setup
	self.name  = name
	self.klass = klass
	self.par   = par
	self.hide  = undefined
	self.want  = undefined
	self.uuid  = listen(function(){self.discover()},
	                    function(m){self.receive(m)})
	self.box   = document.createElement('div')
	self.child = new klass(self.box)
	self.par.appendChild(self.box)
}
