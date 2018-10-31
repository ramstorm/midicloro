
exec { "apt-get update":
  path => "/usr/bin",
}
package { "git":
  ensure  => present,
  require => Exec["apt-get update"],
}
package { "g++":
  ensure  => present,
  require => Exec["apt-get update"],
}
package { "libasound2-dev":
  ensure  => present,
  require => Exec["apt-get update"],
}
package { "libboost-system-dev":
  ensure  => present,
  require => Exec["apt-get update"],
}
package { "libboost-regex-dev":
  ensure  => present,
  require => Exec["apt-get update"],
}
package { "libboost-program-options-dev":
  ensure  => present,
  require => Exec["apt-get update"],
}

user { "midicloro":
      ensure           => "present",
      gid              => "100",
      home             => "/home/midicloro",
      comment           => "midicloro",
      groups            => "users",
	  #Password should go in single quotes!!
      password         => '$1$jHAh4vJQ$h6MEGTQS9C0Avi5l70Cw80',
      password_max_age => "99999",
      password_min_age => "0",
      shell            => "/bin/bash",
      uid              => "501",
}

file { "/home/midicloro":
	require => User["midicloro"],
	ensure => "directory",
	owner => "midicloro",
	group => "users",
	mode => "0755",
}

exec { "git clone https://github.com/csantinc/midicloro.git":
  require => File["/home/midicloro"],
  path => "/usr/bin",
  cwd => "/home/midicloro/",
  user => "midicloro",
}

exec { "make":
  require => Exec["git clone https://github.com/csantinc/midicloro.git"],  
  path => "/usr/bin",
  cwd  => "/home/midicloro/midicloro/",
  user => "midicloro",  
}

