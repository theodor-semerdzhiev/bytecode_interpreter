

class Point(x,y) {
    let x = x;
    let y = y;

    func getPoint() {
        return [x,y];
    }

    func Dist0(x1, y1) {
        return (abs(x1 - x)**2 + abs(y1-y) ** 2) ** (0.5);
    }

    func Dist1(p1) {
        return (abs(p1->x - x)**2 + abs(p1->y - y) ** 2) ** (0.5);
    }
}

let p = Point(1,1);
for(let i = 0 ; i < 20; i = i + 1;) {
    println(p->Dist1(Point(i,3)));
}



