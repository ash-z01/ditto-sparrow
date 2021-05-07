import person for Person

fun fn() {
    var p = Person.new("xiaoming", "male")
    p.sayHi()
}

class Family < Person {
    var father
    var mother
    var child
    new(f, m, c) {
        father = f
        mother = m
        child = c
        super("wbf", "male")
    }
}

var f = Family.new("wbf", "ls", "shine")
f.sayHi()

fn()

