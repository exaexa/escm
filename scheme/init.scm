
;Oh hai. This is ESCM interpreter initialization file, which contains much more
;interesting builtins then you can even imagine! lolz.
;
;It should be located somewhere in your home directory as .escm-init, and when
;not found there, it's searched for in /etc/escm-init.
;
;PS., optimized for size. sry.

(define(*error-hook* . x)(display "error: ")(display x)(newline))

(define(for-each l d)(if(pair? d)(begin(l(car d))(for-each l(cdr d)))()))

(define(cars x)(if(null? x)()(cons(car(car x))(cars(cdr x)))))
(define(cdrs x)(if(null? x)()(cons(cdr(car x))(cdrs(cdr x)))))
(define(map f . x)(if(null?(car x))()(cons(apply f(cars x))
 (apply map(cons f(cdrs x))))))

(define(zero? x)(= x 0))
(define(sym-eq? x y)(zero?(sym-cmp x y)))
(define(str-eq? x y)(zero?(str-cmp x y)))

(macro(and a)(if(null?(cdr a))#t(list 'if(list 'false?(cadr a))#f(cons
 'and(cddr a)))))

(macro(or a)(if(null?(cdr a))#f(list 'if(list 'true?(cadr a))#t(cons 
 'or(cddr a)))))

(define(caar x)(car(car x)))
(define(cadr x)(car(cdr x)))
(define(cdar x)(cdr(car x)))
(define(cddr x)(cdr(cdr x)))
(define(caaar x)(car(car(car x))))
(define(caadr x)(car(car(cdr x))))
(define(cadar x)(car(cdr(car x))))
(define(caddr x)(car(cdr(cdr x))))
(define(cdaar x)(cdr(car(car x))))
(define(cdadr x)(cdr(car(cdr x))))
(define(cddar x)(cdr(cdr(car x))))
(define(cdddr x)(cdr(cdr(cdr x))))
(define(caaaar x)(car(car(car(car x)))))
(define(caaadr x)(car(car(car(cdr x)))))
(define(caadar x)(car(car(cdr(car x)))))
(define(caaddr x)(car(car(cdr(cdr x)))))
(define(cadaar x)(car(cdr(car(car x)))))
(define(cadadr x)(car(cdr(car(cdr x)))))
(define(caddar x)(car(cdr(cdr(car x)))))
(define(cadddr x)(car(cdr(cdr(cdr x)))))
(define(cdaaar x)(cdr(car(car(car x)))))
(define(cdaadr x)(cdr(car(car(cdr x)))))
(define(cdadar x)(cdr(car(cdr(car x)))))
(define(cdaddr x)(cdr(car(cdr(cdr x)))))
(define(cddaar x)(cdr(cdr(car(car x)))))
(define(cddadr x)(cdr(cdr(car(cdr x)))))
(define(cdddar x)(cdr(cdr(cdr(car x)))))
(define(cddddr x)(cdr(cdr(cdr(cdr x)))))
(define(caaaddr x)(car(car(car(cdr(cdr x))))))

(define(cadrs x)(map cadr x))

(macro(let form)(if(symbol?(cadr form))(begin(define vars(caddr form))
 (define name(cadr form))(list 'begin(list 'define name(cons 'lambda(cons
 (cars vars)(cdddr form))))(cons name(cadrs vars))))(begin(define vars(cadr
 form))(cons(cons 'lambda(cons(cars vars)(cddr form)))(cadrs vars)))))

(macro(cond form)(if(null?(cdr form))()(let((a(car form))(c(caadr form))
 (p(cdadr form))(r(cddr form)))(if(and(symbol? c)(sym-eq? c 'else))(cons 
 'begin p)(list 'if c(cons begin p)(cons a r))))))

(define (equal? x y) (cond ((null? x) (null? y)) ((number? x) (and (number? y) (= x y))) ((string? x) (and (string? y) (str-eq? x y))) ((symbol? x) (and (symbol? y) (sym-eq? x y))) ((pair? x) (and (pair? y) (equal? (car x) (car y)) (equal? (cdr x) (cdr y)))) (else (error "unsupported type of args" x y))))

(macro (case form) (if(null?(cddr form))() (if(and(symbol?(caaddr form))(sym-eq? 'else (caaddr form))) (cons 'begin (cdaddr form)) (list 'if (list 'equal? (cadr form) (caaaddr form)) (cons 'begin (cdaddr form)) (cons 'case (cons (cadr form) (cdddr form)))))))

