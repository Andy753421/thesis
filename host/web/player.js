Player = function(par)
{
	var self = this

	// Video
	Video = function(par)
	{
		var self = this

		// Network interface
		self.discover = function(item) {
		}

		self.receive = function(item) {
			if (!item)
				return
			if (item.button && item.button.control == 'start') {
				if (!self.video.src) {
					broadcast({ 'button': {
						'control': 'first',
						'state':   'clicked',
					} })
				} else {
					item.button.control = 'toggle'
				}
			}
			if (item.button && item.button.control == 'toggle') {
				if (self.playing) {
					self.video.pause();
					self.playing = false
				} else {
					self.video.play();
					self.playing = true
				}
			}
			if (item.select && item.select.path) {
				self.video.src = item.select.path
				self.video.load()
				self.video.play()
				self.playing = true
			}
		}

		// Setup
		self.uuid = listen(function(){self.discover()},
		                   function(m){self.receive(m)})

		self.title = document.createElement("h3")
		self.title.innerHTML = "Video Player"
		self.title.style.cssText =
			'margin: 0.5em;'

		self.error = document.createElement("div")
		self.error.innerHTML = "Video not supported"

		self.video = document.createElement("video")
		self.video.style.cssText =
			'background: #000000;' +
			'padding:    0;'       +
			'height:     100%;'    +
			'width:      100%;'
		self.video.appendChild(self.error)
		
		self.par = par;
		self.par.style.cssText =
			'text-align: center;'
		//self.par.appendChild(self.title)
		self.par.appendChild(self.video)
	}

	// Controls
	Controls = function(par)
	{
		var self = this

		// Control buttons
		self.controls = [
			{ 'name': 'first',  'icon': '\u25C0\u25C0' },
			{ 'name': 'prev',   'icon': '\u25C0'  },
			{ 'name': 'toggle', 'icon': '\u25AE \u25AE' },
			{ 'name': 'next',   'icon': '\u25B6'  },
			{ 'name': 'last',   'icon': '\u25B6\u25B6' },
		]

		// Helper functions
		self.setup = function(info) {
			var btn     = document.createElement('input')
			btn.type    = 'button'
			btn.value   = info.icon
			btn.style.cssText =
				'margin: 0 .2em;' +
				'width:  3em;'
			btn.onclick = function() {
				broadcast({ 'button': {
					'control': info.name,
					'state':   'clicked',
				} }, self.uuid)
			}
			self.box.appendChild(btn)
		}

		// Network interface
		self.discover = function(item) {
		}

		// Setup
		self.uuid = listen(function(){self.discover()})
		self.box  = document.createElement("div")
		self.box.style.cssText =
			'text-align: center'
		for (i in self.controls)
			self.setup(self.controls[i])
		par.appendChild(self.box)
	}

	// Playlist
	Playlist = function(par)
	{
		var self = this
		var base = "http://pileus.org/andy/cs239/sim/video/"

		// Video database
		self.database = [
			{ 'title': 'Bright Side of Life',
			  'path':  base + 'always_look_on_the_bright_side_of_life.mp4' },
			{ 'title': 'Dead Parrot',
			  'path':  base + 'dead_parrot.mp4' },
			{ 'title': 'Killer Bunny',
			  'path':  base + 'killer_bunny.mp4' },
			{ 'title': 'Lumberjack Song',
			  'path':  base + 'lumberjack_song.mp4' },
			{ 'title': 'Ministry of Silly Walks',
			  'path':  base + 'ministry_of_silly_walks.mp4' },
			{ 'title': 'Defense Against Fruit',
			  'path':  base + 'self_defense_against_fruit.mp4' },
			{ 'title': 'Spam',
			  'path':  base + 'spam.mp4' },
			{ 'title': 'The Black Knight',
			  'path':  base + 'the_black_knight.mp4' },
		]

		// Helper functions
		self.update = function(index, send) {
			if (self.item) {
				self.item.button.style.backgroundColor = ''
				self.item.button.style.fontWeight      = ''
			}
			if (index == 'stop') {
				self.index  = undefined
				self.button = undefined
			} else {
				//debug('switch: ' + self.index + ' -> ' + index)
				self.index = index
				self.item  = self.database[index]
				self.item.button.style.backgroundColor = '#a0ffa0'
				self.item.button.style.fontWeight      = 'bold'
			}
			if (send) {
				broadcast({ 'select': {
					'title': self.item.title,
					'path':  self.item.path,
				} })
			}
		}

		self.setup = function(index) {
			var item   = self.database[index]
			item.button = document.createElement('input')
			item.button.type  = 'button'
			item.button.value = (index+1)+'. '+item.title
			item.button.style.cssText =
				'display:    relative;' +
				'position:   relative;' +
				'width:      100%;'     +
				'top:        0;'        +
				'bottom:     0;'        +
				'margin:     .3em 0;'   +
				'text-align: left;'
			item.button.onclick = function() {
				self.update(index, true)
			}
			self.box.appendChild(item.button)
		}

		// Network interface
		self.discover = function(item) {
		}

		self.receive = function(item) {
			if (!item)
				return
			if (!item.button)
				return
			if (item.button.state != 'clicked')
				return
			if (!item.button.control)
				return
			var last  = self.database.length - 1
			var index = {
				'first': 0,
				'prev':  Math.max(self.index-1, 0),
				'next':  Math.min(self.index+1, last),
				'last':  last,
			}[item.button.control]
			if (index === undefined || index === NaN)
				return
			self.update(index, true)
		}

		// Setup
		self.par  = par
		self.uuid = listen(function(){self.discover()},
		                   function(m){self.receive(m)})
		self.box  = document.createElement('div')
		self.box.innerHTML     = "<p>Playlist</p>"
		self.box.style.cssText = 'text-align: center;'
		for (i in self.database)
			self.setup(parseInt(i))
		self.par.appendChild(self.box)
	}

	// Delete
	self.del = function() {
		debug('Player.del')
		if (self.video.del)    self.video.del()
		if (self.controls.del) self.controls.del()
		if (self.playlist.del) self.playlist.del()
	}

	// Properties
	self.par = par

	var top_box  = document.createElement("div")
	var lst_box  = document.createElement("div")
	var main_box = document.createElement("div")
	var vid_box  = document.createElement("div")
	var ctl_box  = document.createElement("div")

	lst_box.style.cssText =
		//"float:          right;"               +
		'width:          8em;'                 +
		'padding:        1em 0.5em 2em 0.5em;' +
		'background:     #f8f8f8;'             +
		'border-left:    solid 1px #dddddd;'   +
		'border-bottom:  solid 1px #dddddd;'
	main_box.appendChild(ctl_box)
	main_box.appendChild(vid_box)
	top_box.appendChild(main_box)
	top_box.appendChild(lst_box)

	self.video         = new Virtual('Video', Video, vid_box)
	self.video.hide    = vid_box
	self.video.want    = 'projector'

	self.controls      = new Virtual('Controls', Controls, ctl_box)
	self.controls.hide = ctl_box
	self.controls.want = 'tablet'

	self.playlist      = new Virtual('Playlist', Playlist, lst_box)
	self.playlist.hide = lst_box
	self.playlist.want = 'phone'


	self.par.appendChild(top_box)
}
