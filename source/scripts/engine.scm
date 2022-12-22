(define-macro (time expr)
    `(let ((start (*s7* 'cpu-time)))
       (let ((res (list ,expr)))
         (list (car res)
       (- (*s7* 'cpu-time) start)))))

(define-macro (funcinfo func info)
    `(*function* (funclet ,func) ,info))

(define-macro (funcloc func)
    `(format #f "~A:~A" (funcinfo ,func 'file) (funcinfo ,func 'line)))

(define-macro (funcsrc func)
    `(funcinfo ,func 'source))

(define (objects->string . objs)
    (apply string-append (map (lambda (obj) (object->string obj #f)) objs)))

(define-bacro (glog data lvl)
    (let ((file (port-filename))
          (line (port-line-number)))
       `(log ,lvl ,data
           (if (equal? ,file "*stdin*") (*function* (outlet (curlet)) 'file) ,file)
	   (if (equal? ,file "*stdin*") (*function* (outlet (curlet)) 'line) ,line))))

(define-bacro (loginfo . data)
    `(glog (objects->string ,@data) 0))

(define-bacro (logwarn . data)
    `(glog (objects->string ,@data) 1))
    
(define-bacro (logerr . data)
    `(glog (objects->string ,@data) 2))
    
(define-bacro (logcrit . data)
    `(glog (objects->string ,@data) 3))

(define (set_fps fps) (settings_cmd 0 (/ 1 fps)))
(define (set_update fps) (settings_cmd 1 (/ 1 fps)))
(define (set_phys fps) (settings_cmd 2 (/ 1 fps)))

(define (win_fulltoggle w) (win_cmd w 0))
(define (win_fullscreen w) (win_cmd w 1))
(define (win_unfullscreen w) (win_cmd w 2))
(define (win_title s) (win_cmd 0 3 s))

(define (load_level s) (gen_cmd 0 s))
(define (load_prefab s) (gen_cmd 1 s))
(define (quit) (sys_cmd 0))
(define (exit) (quit))

(define (sound_play sound) (sound_cmd sound 0))
(define (sound_pause sound) (sound_cmd sound 1))
(define (sound_stop sound) (sound_cmd sound 2))
(define (sound_restart sound) (sound_cmd sound 3))

(define-macro (registertype type . expr)
    (let ((f (gensym)))
        `(begin
	    (define (,f) (begin . ,expr))
	    (register ,(case type
	                 ((update) 0)
			 ((gui) 1)) ,f))))

(define-macro (update . expr)
    `(registertype update ,@expr))

(define-macro (gui . expr)
    `(registertype gui ,@expr))
    
(define-macro (while condition . body)
    (let ((loop (gensym)))
    `(let ,loop ()
        (cond (,condition
	(begin . ,body)
	(,loop))))))

(define (clamp val min max)
    (cond ((< val min) min)
          ((> val max) max)
	  (else val)))

(define (lerp s f dt)
    (+ s (* (clamp dt 0 1) (- f s))))

(define (deg2rad deg)
    (* deg 0.01745329252))

(define (body_angle! body angle) (set_body body 0 angle))
(define (body_pos! body x y) (set_body_pos body 0 x y))
(define (body_move! body x y) (set_body_pos body 1 x y))

(define (gravity! x y) (phys_set 0 x y))

(define (b2i val) (if (eq? val #f) 0 1))
(define (dbg_draw_phys val) (settings_cmd 3 (b2i val)))
(define (sim_play) (sys_cmd 1))
(define (sim_stop) (sys_cmd 2))
(define (sim_pause) (sys_cmd 3))
(define (sim_step) (sys_cmd 4))
(define (sim_play?) (sys_cmd 5))
(define (sim_pause?) (sys_cmd 6))

(define (bodytype? body) (phys_q body 0))

(define-macro (register-phys type body . expr)
    (let ((f (gensym)))
        `(begin
	    (define (,f) (begin . ,expr))
	    (phys_cmd ,body ,(case type
	                  ((collide) 0)
			  ((separate) 3)) ,f))))

(define-macro (collide . expr)
    `(register-phys collide body ,@expr))

(define-macro (separate . expr)
    `(register-phys separate body ,@expr))

(define-macro (not! var)
    `(set! ,var (not ,var)))

(define-macro* (define-script name (lets ()) . expr )
    `(define* (,name (gameobject -1))
        (let ((body gameobject)
	     ,@lets)
	  ,@expr
	  (curlet))))
